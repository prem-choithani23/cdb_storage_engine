//
// Created by prem-choithani on 8/21/26.
//

#include "../../include/buffer/buffer_pool.h"

#include <stdlib.h>
#include <string.h>

#include "../../include/storage/slotted_page.h"
#include "../../include/logging/logger.h"

static const char * from = "BUFFER_POOL";


BufferPool * buffer_pool_open(const char * path) {

    logger(from, "Initializing buffer_pool_open...");

    BufferPool * buffer_pool =
        (BufferPool*)malloc(sizeof(BufferPool));

    buffer_pool->clock_hand = 0;

    logger(from, "Creating page table.");

    buffer_pool->page_table =
        page_table_create(PAGE_TABLE_CAPACITY);

    logger(from, "Initializing buffer frames.");

    memset(
        buffer_pool->buffer_frames,
        0,
        sizeof(BufferFrame) * BUFFER_POOL_SIZE
    );

    logger(from, "Opening disk manager.");

    buffer_pool->dm =
        disk_manager_open(path);

    logger(from, "Buffer pool opened successfully.");

    return buffer_pool;
}


void buffer_pool_close(BufferPool* bp) {

    logger(from, "Initializing buffer_pool_close...");

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {

        BufferFrame* frame =
            &bp->buffer_frames[i];

        if (frame->is_occupied && frame->is_dirty) {

            logger(
                from,
                "Dirty page found. Flushing page to disk."
            );

            disk_write_page(
                bp->dm,
                frame->page_id,
                &frame->page
            );
        }
    }

    logger(from, "Freeing page table.");

    page_table_free(bp->page_table);

    logger(from, "Closing disk manager.");

    disk_manager_close(bp->dm);

    free(bp);

    logger(from, "Buffer pool closed successfully.");
}


FrameId clock_evict(BufferPool * bf) {

    logger(from, "Initializing clock_evict...");

    while (true) {

        BufferFrame * frame =
            &bf->buffer_frames[bf->clock_hand];

        logger(from, "Inspecting current buffer frame.");

        if (frame->pin_count > 0) {

            logger(
                from,
                "Frame is pinned. Skipping frame."
            );

            bf->clock_hand =
                (bf->clock_hand + 1) % BUFFER_POOL_SIZE;

            continue;
        }

        if (frame->ref_bit == 1) {

            logger(
                from,
                "Reference bit is set. Giving frame a second chance."
            );

            frame->ref_bit = 0;

            bf->clock_hand =
                (bf->clock_hand + 1) % BUFFER_POOL_SIZE;

            continue;
        }

        if (frame->is_occupied) {

            logger(
                from,
                "Occupied frame selected as eviction candidate."
            );

            if (frame->is_dirty) {

                logger(
                    from,
                    "Eviction candidate is dirty. Writing page to disk."
                );

                disk_write_page(
                    bf->dm,
                    frame->page_id,
                    &frame->page
                );

                frame->is_dirty = false;

                logger(
                    from,
                    "Dirty page written successfully."
                );
            }

            logger(
                from,
                "Removing evicted page from page table."
            );

            page_table_delete(
                bf->page_table,
                frame->page_id
            );
        }

        FrameId victim = bf->clock_hand;

        logger(
            from,
            "Victim frame selected successfully."
        );

        bf->clock_hand =
            (bf->clock_hand + 1) % BUFFER_POOL_SIZE;

        return victim;
    }
}


Page * buffer_pool_fetch_page(
    BufferPool * bf,
    PageId page_id
) {

    logger(from, "Initializing buffer_pool_fetch_page...");

    FrameId frame_id =
        page_table_lookup(
            bf->page_table,
            page_id
        );

    Page page;

    if (frame_id == INVALID_FRAME_ID) {

        logger(
            from,
            "Page not found in buffer pool. Cache miss."
        );

        FrameId victim =
            clock_evict(bf);

        logger(
            from,
            "Reading requested page from disk."
        );

        disk_read_page(
            bf->dm,
            page_id,
            &page
        );

        BufferFrame *buffer_frame =
            &bf->buffer_frames[victim];

        buffer_frame->ref_bit = 1;
        buffer_frame->pin_count++;
        buffer_frame->page_id = page_id;
        buffer_frame->is_occupied = true;
        buffer_frame->is_dirty = false;
        buffer_frame->page = page;

        logger(
            from,
            "Loaded page into buffer frame."
        );

        page_table_insert(
            bf->page_table,
            page_id,
            victim
        );

        logger(
            from,
            "Inserted page-to-frame mapping into page table."
        );

        return &buffer_frame->page;
    }

    logger(
        from,
        "Page found in buffer pool. Cache hit."
    );

    BufferFrame *buffer_frame =
        &bf->buffer_frames[frame_id];

    buffer_frame->ref_bit = 1;

    logger(
        from,
        "Reference bit set for fetched page."
    );

    return &buffer_frame->page;
}


void buffer_pool_unpin_page(
    BufferPool * bf,
    PageId page_id,
    bool is_dirty
) {

    logger(from, "Initializing buffer_pool_unpin_page...");

    FrameId victim =
        page_table_lookup(
            bf->page_table,
            page_id
        );

    if (victim == INVALID_FRAME_ID) {

        logger(
            from,
            "Page not found in buffer pool. Cannot unpin page."
        );

        return;
    }

    BufferFrame * buffer_frame =
        &bf->buffer_frames[victim];

    if (buffer_frame->pin_count > 0) {

        buffer_frame->pin_count--;

        logger(
            from,
            "Page unpinned successfully."
        );
    }

    if (is_dirty) {

        buffer_frame->is_dirty = true;

        logger(
            from,
            "Page marked as dirty."
        );
    }

    return;
}


bool buffer_pool_flush_page(
    BufferPool * bf,
    PageId page_id
) {

    logger(from, "Initializing buffer_pool_flush_page...");

    FrameId victim =
        page_table_lookup(
            bf->page_table,
            page_id
        );

    if (victim == INVALID_FRAME_ID) {

        logger(
            from,
            "Page not found in buffer pool. Flush failed."
        );

        return false;
    }

    BufferFrame * buffer_frame =
        &bf->buffer_frames[victim];

    logger(
        from,
        "Writing page from buffer pool to disk."
    );

    disk_write_page(
        bf->dm,
        page_id,
        &buffer_frame->page
    );

    buffer_frame->is_dirty = false;

    logger(
        from,
        "Page flushed successfully and marked clean."
    );

    return true;
}


Page * buffer_pool_new_page(
    BufferPool * bf,
    PageId * out_page_id
) {

    logger(from, "Initializing buffer_pool_new_page...");

    PageId page_id =
        disk_allocate_page(bf->dm);

    logger(
        from,
        "New page allocated on disk."
    );

    FrameId victim =
        clock_evict(bf);

    logger(
        from,
        "Buffer frame selected for new page."
    );

    *out_page_id = page_id;

    Page page;

    page_init(
        &page,
        page_id
    );

    logger(
        from,
        "Initialized new page."
    );

    BufferFrame * frame =
        &bf->buffer_frames[victim];

    frame->ref_bit = 1;
    frame->pin_count = 1;
    frame->page_id = page_id;
    frame->is_occupied = true;
    frame->is_dirty = false;
    frame->page = page;

    logger(
        from,
        "New page loaded into buffer frame."
    );

    page_table_insert(
        bf->page_table,
        page_id,
        victim
    );

    logger(
        from,
        "Inserted new page into page table."

    );

    logger(
        from,
        "New page created successfully."
    );

    return &frame->page;
}