# Storage Engine Project

## Overview

This repository contains a simple page-based storage engine written in C. The engine is designed around the following core layers:

- `headers/page.h` and `layers/page.c`
  - Defines the page structure and implements basic free-space accounting.
  - Pages contain a fixed-size header and a payload buffer for record storage.

- `headers/slot.h` and `headers/slotted_page.h` / `layers/slotted_page.c`
  - Implements a slotted page layout where slot metadata is stored at the front of the page and record payload is stored at the back.
  - Supports insert, read, update, and delete operations on records within a page.

- `headers/disk_manager.h` / `layers/disk_manager.c`
  - Manages page-level I/O against a file acting as the storage medium.
  - Handles page reads, writes, and page allocation.

- `headers/heap_file.h` / `layers/heap_file.c`
  - Implements a heap file abstraction that stores records across pages.
  - Tracks free pages and uses the slotted page layer for record placement.
  - Supports insertion, reading, updating, and deletion through `TID` handles.

- `headers/table_api.h` / `layers/table_api.c`
  - Provides a thin API wrapper over heap file operations.
  - Exposes `insert_data`, `read_data`, `update_data`, and `delete_data`.

- `headers/logger.h` / `layers/logger.c`
  - Provides a small logging utility controlled by compile-time configuration.
  - Used throughout the engine for status and debugging messages.

- `headers/constants.h`
  - Defines page sizes, invalid identifiers, and other configuration constants.

- `headers/data/student.h`
  - Defines the `Student` record used by the sample application and test harness.

## Project Structure

- `main.c`
  - Drives a set of sample storage operations against the Table API.
  - Inserts, updates, deletes, and verifies student records.

- `test.c`
  - Contains a regression test suite for the storage engine.
  - Validates TID stability, update behavior, deletion semantics, and bulk record management.

- `benchmark/benchmark.c`
  - Compares simple non-paged file I/O against the page-based engine.
  - Measures insertion and random read performance for both approaches.

- `scripts/run_main.sh`
  - Compiles the engine sources and runs `main.c`.
  - Deletes `database.db` before execution to ensure a fresh run.

- `scripts/run_test.sh`
  - Compiles the engine sources and runs `test.c`.
  - Deletes `database.db` before execution so tests start with a clean state.

- `scripts/run_benchmark.sh`
  - Compiles the engine sources and runs `benchmark/benchmark.c`.
  - Deletes and recreates the benchmark database files in `databases/normal_database.db` and `databases/page_database.db` before running.

- `executables/`
  - Produced runtime executables are written here by the shell scripts.

## How the Shell Scripts Work

The scripts are responsible for compilation and execution. They all use `gcc` with the following options:

- `-std=c11`
- `-Wall`
- `-Wextra`
- `-Iheaders`

### Output Executables

The compiled binaries are written to:

- `executables/main_exec`
- `executables/test_exec`
- `executables/benchmark_exec`

This keeps runtime artifacts separate from source files.

## Running the Project

From the repository root, run one of the scripts:

```bash
./scripts/run_main.sh
./scripts/run_test.sh
./scripts/run_benchmark.sh
```

### Main Application

- Uses the table API to insert, read, update, and delete sample `Student` records.
- `database.db` is removed before every run to guarantee a clean start.

### Regression Tests

- Runs a suite of storage engine tests in `test.c`.
- Ensures deleted records cannot be read back and updates behave correctly.
- `database.db` is removed before every test run.

### Benchmark

- Compares traditional sequential storage in `databases/normal_database.db` with the page-based engine using `databases/page_database.db`.
- Both benchmark databases are recreated before execution.

## Notes

- Runtime files such as `database.db` and the `databases/` folder are intentionally not committed.
- The shell scripts guarantee that each run begins with fresh storage files.
- This repository is primarily a learning and demonstration engine rather than a production-grade database.

## Recommended Workflow

1. Inspect the project layers in `headers/` and `layers/`.
2. Run `./scripts/run_test.sh` to validate behavior.
3. Run `./scripts/run_main.sh` to see the example usage.
4. Run `./scripts/run_benchmark.sh` to compare performance.
