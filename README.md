# Mini Storage Engine in C

A page-based storage engine written from scratch in C.

This project implements the core internals of a relational database storage layer — the part that sits below SQL, below query planning, below indexes — the part that actually puts bytes on disk and gets them back reliably. It is a minimal but real implementation modeled after concepts from PostgreSQL's heap storage and InnoDB's buffer pool manager.

---

## What It Is

Most developers interact with databases through queries. This project is the layer those queries never expose — the one that decides which page a record lives on, how to find it again in constant time, how to keep hot pages in memory, and how to avoid unnecessary disk I/O.

It is not a toy. It implements real database internals:

- Slotted page layout with slot indirection
- TID-based record addressing (page_id, slot_id)
- Heap file with free-space linked list
- Buffer pool with Clock eviction and dirty page tracking
- Variable-length record support with in-place and relocating updates
- Soft deletes via tombstoning

It is also not production software. It is single-threaded, has no query language, no crash recovery, and no indexes yet. It is the foundation those things are built on.

---

## Architecture

This project is organized into five layers. Each layer only knows about the layer directly below it. No layer reaches past its neighbor.

```
  main.c / Table API
        │
     HeapFile
        │
    BufferPool
        │
    SlottedPage
        │
    DiskManager
        │
     OS (pread / pwrite)
```

### Disk Manager
The only layer that touches the OS. It holds an open file descriptor and provides three operations: read a page, write a page, allocate a new page. All I/O goes through `pread` and `pwrite` for precise byte-level control. Nothing above this layer knows what a file descriptor is.

### Slotted Page
Manages records within a single 4096-byte page. A page is laid out as a header, a slot array growing downward from the top, and records growing upward from the bottom. Free space is the gap between them.

```
+------------------+
| PageHeader       |  page_id, slot_count, free_space_pointer, next_free_page
+------------------+
| Slot 0           |  offset + length of record 0
| Slot 1           |  offset + length of record 1
| ...              |
|   FREE SPACE     |
| ...              |
| Record 1         |
| Record 0         |
+------------------+
```

Slots provide indirection — when a record moves within a page (due to an update), its slot index stays the same. External references (TIDs) remain valid. A deleted record is tombstoned by setting its slot length to zero.

### Buffer Pool
Sits between the heap file and the disk manager. Maintains a fixed array of page-sized frames in memory. When the heap file needs a page, the buffer pool checks if it is already in a frame (cache hit) or loads it from disk (page fault).

Key concepts:
- **Pinning** — a frame being actively used cannot be evicted. Pin count tracks active callers. Eviction skips pinned frames.
- **Dirty tracking** — a modified page is marked dirty on unpin. Dirty pages are flushed to disk before eviction or on pool close. Clean pages are simply overwritten.
- **Clock eviction** — a circular sweep with a reference bit per frame. On access, the bit is set to 1. On eviction sweep, a bit of 1 is cleared (second chance) and the hand moves on. A bit of 0 means the frame is the eviction victim.
- **Page table** — a chaining hash table mapping `PageId → frame_index` for O(1) lookup.

### Heap File
Manages a collection of pages as a table. Maintains a linked list of pages with free space via `next_free_page` in each page header. On insert, walks the list to find a page with enough contiguous free space. When all pages are full, allocates a new one through the buffer pool.

Returns a TID `(page_id, slot_id)` on every insert — the stable address of the record.

### Table API
The outermost layer. Wraps heap file operations with record-type-aware functions. This is where `insert_student`, `read_student`, `update_student`, `delete_student` live. It knows about `Student` structs. Nothing below it does.

---

## TID — Tuple Identifier

Every record in This project has a TID:

```c
typedef struct {
    PageId page_id;
    SlotId slot_id;
} TID;
```

Six bytes. Stable across in-page updates because the slot array provides indirection — when a record moves within a page, only its slot entry changes, not its slot index. External references never break.

This is the same concept PostgreSQL calls a CTID. It is the foundation that makes B+ Tree indexes cheap to maintain — index leaves store `(key, TID)` pairs, and TID stability means the index never needs updating when a record moves within its page.

---

## Project Structure

```
storage-engine/
│
├── include/
│   ├── buffer/         buffer_pool.h, buffer_frame.h, page_table.h
│   ├── common/         config.h, constants, shared types
│   ├── data/           student.h (record schema)
│   ├── disk/           disk_manager.h
│   ├── logging/        logger.h
│   └── storage/        page.h, slot.h, slotted_page.h, heap_file.h, table_api.h
│
├── src/
│   ├── buffer/         buffer_pool.c, page_table.c
│   ├── disk/           disk_manager.c
│   ├── logging/        logger.c
│   └── storage/        slotted_page.c, heap_file.c, table_api.c
│
├── benchmark/          benchmark.c — flat file vs paged engine comparison
├── tests/              test suite (TID stability, CRUD, stress)
├── scripts/
│   ├── run_main.sh     run the main demo
│   ├── run_test.sh     run the full test suite
│   ├── run_benchmark.sh  run the benchmark
│   └── buffer_pool_test.sh
│
├── notes/              design notes and phase documentation
├── main.c
└── CMakeLists.txt
```

---

## Building

```bash
mkdir build && cd build
cmake ..
make
```

---

## Running

```bash
# Run the demo
./scripts/run_main.sh

# Run the test suite
./scripts/run_test.sh

# Run the benchmark (paged vs flat file)
./scripts/run_benchmark.sh
```

---

## Configuration

Key constants in `include/common/config.h`:

```c
#define PAGE_SIZE          4096
#define BUFFER_POOL_SIZE   64      // number of frames in the buffer pool
#define PAGE_TABLE_CAPACITY 128    // hash table bucket count
#define LOGGING_ENABLED    0       // set to 1 for verbose layer-by-layer logs
```

Increasing `BUFFER_POOL_SIZE` improves random read performance when the working set fits in the pool. The benchmark demonstrates this tradeoff directly.

---

## Benchmark

This project includes a benchmark comparing the paged engine against a flat-file implementation (fixed-size records, `fseek`-based access) across two workloads:

- Sequential insert of N students
- Random read of N students (shuffled IDs)

**Expected result:**
- Flat file wins on sequential inserts — zero per-record overhead
- Paged engine wins on random reads — buffer pool keeps hot pages resident, amortizing disk I/O across multiple records per page

Run `./scripts/run_benchmark.sh` to see results on your machine.

---

## Test Suite

The test suite covers:

- TID stability — deleted TIDs must not return data; new inserts must get distinct TIDs
- Basic CRUD — insert, read, update (in-place and relocating), delete
- Stress test — 10 inserts, mixed deletes, updates, and re-inserts; all living records verified correct, all deleted TIDs verified invalid

---

## What Is Not Here Yet

- **B+ Tree index** — index leaves would store `(key, TID)` pairs, traversal would return a TID for direct page lookup. The TID stability guarantee makes this cheap to maintain.
- **Write-Ahead Logging (WAL)** — crash recovery. The `lsn` field is stubbed in the design but not implemented.
- **LRU eviction** — the buffer pool currently uses Clock. LRU is planned as a configurable alternative via `config.h`.
- **Concurrency** — single-threaded. No latches, no lock manager.
- **Query layer** — no SQL, no parser, no planner.

---


