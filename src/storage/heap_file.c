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


TID heap_file_insert(
    HeapFile* hf,
    void* data,
    uint16_t length
) {

    logger(from, "Initializing heap_file_insert...");

    if (hf->first_free_page == INVALID_PAGE_ID) {

        logger(
            from,
            "No free page found. Allocating a new page."
        );

        buffer_pool_new_page(
            hf->buffer_pool,
            &hf->first_free_page
        );

        logger(
            from,
            "New page allocated and added as first free page."
        );

        buffer_pool_unpin_page(
            hf->buffer_pool,
            hf->first_free_page,
            false
        );

        logger(
            from,
            "Newly allocated page unpinned."
        );
    }

    PageId current_page_id =
        hf->first_free_page;

    PageId previous_page_id =
        INVALID_PAGE_ID;

    while (current_page_id != INVALID_PAGE_ID) {

        logger(
            from,
            "Fetching current free page from buffer pool."
        );

        Page *page =
            buffer_pool_fetch_page(
                hf->buffer_pool,
                current_page_id
            );

        logger(
            from,
            "Current free page fetched successfully."
        );

        int free_space =
            get_free_space(page);

        logger(
            from,
            "Calculated available free space."
        );

        if (free_space >= length) {

            logger(
                from,
                "Sufficient free space found. Inserting record."
            );

            SlotId slot_id =
                page_insert(
                    page,
                    data,
                    length
                );

            logger(
                from,
                "Record inserted into current page."
            );

            buffer_pool_flush_page(
                hf->buffer_pool,
                page->header.page_id
            );

            logger(
                from,
                "Record inserted and page written to disk."
            );

            if (get_free_space(page) < MIN_FREE_SPACE) {

                logger(
                    from,
                    "Page no longer qualifies as a free page. Removing it from free page list."
                );

                if (previous_page_id == INVALID_PAGE_ID) {

                    hf->first_free_page =
                        page->header.next_free_page;

                    logger(
                        from,
                        "Updated head of free page list."
                    );

                }
                else {

                    logger(
                        from,
                        "Fetching previous free page."
                    );

                    Page *prev_page =
                        buffer_pool_fetch_page(
                            hf->buffer_pool,
                            previous_page_id
                        );

                    logger(
                        from,
                        "Previous free page fetched successfully."
                    );

                    prev_page->header.next_free_page =
                        page->header.next_free_page;

                    logger(
                        from,
                        "Updated previous page to bypass current page."
                    );

                    buffer_pool_unpin_page(
                        hf->buffer_pool,
                        previous_page_id,
                        true
                    );

                    logger(
                        from,
                        "Previous free page unpinned and marked dirty."
                    );
                }
            }

            logger(
                from,
                "Unpinning current page after insertion."
            );

            buffer_pool_unpin_page(
                hf->buffer_pool,
                page->header.page_id,
                true
            );

            logger(
                from,
                "heap_file_insert completed successfully."
            );

            return (TID){
                .slot_id = slot_id,
                .page_id = page->header.page_id
            };
        }

        logger(
            from,
            "Current page does not have enough free space. Moving to next free page."
        );

        previous_page_id =
            page->header.page_id;

        current_page_id =
            page->header.next_free_page;

        buffer_pool_unpin_page(
            hf->buffer_pool,
            page->header.page_id,
            false
        );

        logger(
            from,
            "Current page unpinned after read-only traversal."
        );
    }

    logger(
        from,
        "No suitable page found. Allocating a new page and retrying insertion."
    );

    PageId new_page_id;

    buffer_pool_new_page(
        hf->buffer_pool,
        &new_page_id
    );

    logger(
        from,
        "New page allocated successfully."
    );

    buffer_pool_unpin_page(
        hf->buffer_pool,
        new_page_id,
        false
    );

    logger(
        from,
        "Newly allocated page unpinned."
    );

    hf->first_free_page =
        new_page_id;

    logger(
        from,
        "Updated first free page pointer."
    );

    return heap_file_insert(
        hf,
        data,
        length
    );
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
