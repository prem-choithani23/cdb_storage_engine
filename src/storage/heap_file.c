//
// Created by prem-choithani on 7/1/26.
//

#include "../../include/storage/heap_file.h"
#include "../../include/logging/logger.h"
#include "../../include/common/config.h"

#include <stdlib.h>

static const char * from = "HEAP_FILE";


HeapFile* heap_file_open(const char* path) {

    logger(from, "Initializing heap_file_open...");

    HeapFile* hf = (HeapFile*) malloc(sizeof(HeapFile));

    hf->first_free_page = INVALID_PAGE_ID;

    logger(from, "Opening buffer pool.");

    hf->buffer_pool = buffer_pool_open(path);

    logger(from, "Heapfile opened successfully.");

    return hf;
}


void heap_file_close(HeapFile* hf) {

    logger(from, "Initializing heap_file_close...");

    logger(from, "Closing buffer pool.");

    buffer_pool_close(hf->buffer_pool);

    free(hf);

    logger(from, "Heapfile closed successfully.");
}


TID heap_file_insert(HeapFile* hf, void* data, uint16_t length) {
    // ... [Keep allocation logic the same] ...

    PageId current_page_id = hf->first_free_page;
    PageId previous_page_id = INVALID_PAGE_ID;

    while (current_page_id != INVALID_PAGE_ID) {
        Page *page = buffer_pool_fetch_page(hf->buffer_pool, current_page_id);
        int free_space = get_free_space(page);

        if (free_space >= length) {
            // Insert record into memory frame directly
            SlotId slot_id = page_insert(page, data, length);

            // REMOVED: buffer_pool_flush_page call that bypassed the buffer pool!

            if (get_free_space(page) < MIN_FREE_SPACE) {
                if (previous_page_id == INVALID_PAGE_ID) {
                    hf->first_free_page = page->header.next_free_page;
                } else {
                    Page *prev_page = buffer_pool_fetch_page(hf->buffer_pool, previous_page_id);
                    prev_page->header.next_free_page = page->header.next_free_page;
                    buffer_pool_unpin_page(hf->buffer_pool, previous_page_id, true);
                }
            }

            // Pin dropped, marked as dirty. Physical disk commit is deferred to clock eviction!
            buffer_pool_unpin_page(hf->buffer_pool, page->header.page_id, true);

            return (TID){.slot_id = slot_id, .page_id = page->header.page_id};
        }

        previous_page_id = page->header.page_id;
        current_page_id = page->header.next_free_page;
        buffer_pool_unpin_page(hf->buffer_pool, page->header.page_id, false);
    }

    // ... [Keep retry path allocation identical] ...
    PageId new_page_id;
    buffer_pool_new_page(hf->buffer_pool, &new_page_id);
    buffer_pool_unpin_page(hf->buffer_pool, new_page_id, false);
    hf->first_free_page = new_page_id;

    return heap_file_insert(hf, data, length);
}



int heap_file_read(
    HeapFile* hf,
    TID tid,
    void* out,
    uint16_t length
) {

    logger(from, "Starting heap_file_read()...");

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    logger(
        from,
        "Fetching requested page from buffer pool."
    );

    Page *page =
        buffer_pool_fetch_page(
            hf->buffer_pool,
            page_id
        );

    logger(
        from,
        "Requested page fetched successfully."
    );

    logger(
        from,
        "Reading record from page."
    );

    int result =
        page_read(
            page,
            slot_id,
            out,
            length
        );

    logger(
        from,
        "Record read operation completed."
    );

    buffer_pool_unpin_page(
        hf->buffer_pool,
        page->header.page_id,
        false
    );

    logger(
        from,
        "Page unpinned after read-only operation."
    );

    logger(
        from,
        "heap_file_read completed successfully."
    );

    return result;
}


void heap_file_delete(
    HeapFile* hf,
    TID tid
) {

    logger(
        from,
        "Starting heap_file_delete()..."
    );

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    logger(
        from,
        "Fetching page containing record."
    );

    Page *page =
        buffer_pool_fetch_page(
            hf->buffer_pool,
            page_id
        );

    logger(
        from,
        "Page fetched successfully."
    );

    logger(
        from,
        "Deleting record from page."
    );

    page_delete(
        page,
        slot_id
    );

    logger(
        from,
        "Record deleted successfully."
    );

    logger(
        from,
        "Adding page back to free page list."
    );

    page->header.next_free_page =
        hf->first_free_page;

    hf->first_free_page =
        page_id;

    logger(
        from,
        "Updated free page list."
    );

    buffer_pool_unpin_page(
        hf->buffer_pool,
        page->header.page_id,
        true
    );

    logger(
        from,
        "Deleted page unpinned and marked dirty."
    );

    logger(
        from,
        "heap_file_delete completed successfully."
    );
}


TID heap_file_update(
    HeapFile* hf,
    TID tid,
    void* data,
    uint16_t length
) {

    logger(
        from,
        "Starting heap_file_update()..."
    );

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    logger(
        from,
        "Fetching page containing record."
    );

    Page *page =
        buffer_pool_fetch_page(
            hf->buffer_pool,
            page_id
        );

    logger(
        from,
        "Page fetched successfully."
    );

    logger(
        from,
        "Attempting to update record."
    );

    SlotId post_update_slot_id =
        page_update(
            page,
            slot_id,
            data,
            length
        );

    if (post_update_slot_id != INVALID_SLOT_ID) {

        logger(
            from,
            "Update completed successfully in existing page."
        );

        buffer_pool_unpin_page(
            hf->buffer_pool,
            page->header.page_id,
            true
        );

        logger(
            from,
            "Updated page unpinned and marked dirty."
        );

        logger(
            from,
            "heap_file_update completed successfully."
        );

        return (TID){
            .slot_id = post_update_slot_id,
            .page_id = page_id
        };
    }

    logger(
        from,
        "Starting OUT-OF-PAGE-RELOCATION."
    );

    buffer_pool_unpin_page(
        hf->buffer_pool,
        page_id,
        false
    );

    logger(
        from,
        "Original page unpinned after failed in-place update."
    );

    logger(
        from,
        "Deleting old record from original page."
    );

    heap_file_delete(
        hf,
        tid
    );

    logger(
        from,
        "Old record deleted successfully."
    );

    logger(
        from,
        "Inserting updated record into another page."
    );

    TID newTid =
        heap_file_insert(
            hf,
            data,
            length
        );

    logger(
        from,
        "Updated record inserted into new page."
    );

    logger(
        from,
        "Page successfully updated through relocation."
    );

    return newTid;
}
