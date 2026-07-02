//
// Created by prem-choithani on 7/1/26.
//

#include "../headers/heap_file.h"
#include "../headers/logger.h"
#include "../headers/constants.h"

#include <stdlib.h>

static const char * from = "HEAP_FILE";

HeapFile* heap_file_open(const char* path) {

    logger(from , "Opening Heapfile");
    HeapFile* hf = (HeapFile*) malloc(sizeof(HeapFile));
    hf->first_free_page = INVALID_PAGE_ID;
    hf->dm = disk_manager_open(path);
    logger(from , "Heapfile opened successfully");

    return hf;
}

void heap_file_close(HeapFile* hf) {
    disk_manager_close(hf->dm);
    free(hf);
}

TID heap_file_insert(HeapFile* hf, void* data, uint16_t length) {

    logger(from, "Initializing heap_file_insert...");

    if (hf->first_free_page == INVALID_PAGE_ID) {
        logger(from, "No free page found. Allocating a new page.");
        hf->first_free_page = disk_allocate_page(hf->dm);
    }

    PageId current_page_id = hf->first_free_page;
    PageId previous_page_id = INVALID_PAGE_ID;

    while (current_page_id != INVALID_PAGE_ID) {

        logger(from, "Reading current page.");

        Page page;
        disk_read_page(hf->dm, current_page_id, &page);

        int free_space = get_free_space(&page);

        if (free_space >= length) {
            logger(from, "Sufficient free space found. Inserting record.");

            SlotId slot_id = page_insert(&page, data, length);

            disk_write_page(hf->dm, page.header.page_id, &page);

            logger(from, "Record inserted and page written to disk.");

            if (get_free_space(&page) < MIN_FREE_SPACE) {

                logger(from, "Page no longer qualifies as a free page. Removing it from free page list.");

                if (previous_page_id == INVALID_PAGE_ID) {
                    hf->first_free_page = page.header.next_free_page;
                    logger(from, "Updated head of free page list.");
                }
                else {
                    Page prev_page;
                    disk_read_page(hf->dm, previous_page_id, &prev_page);

                    prev_page.header.next_free_page = page.header.next_free_page;

                    disk_write_page(hf->dm, previous_page_id, &prev_page);

                    logger(from, "Updated previous page to bypass current page in free page list.");
                }
            }

            logger(from, "heap_file_insert completed successfully.");

            return (TID){ .slot_id = slot_id, .page_id = page.header.page_id };
        }

        logger(from, "Current page does not have enough free space. Moving to next free page.");

        previous_page_id = page.header.page_id;
        current_page_id = page.header.next_free_page;
    }

    logger(from, "No suitable page found. Allocating a new page and retrying insertion.");

    PageId new_page_id = disk_allocate_page(hf->dm);
    hf->first_free_page = new_page_id;

    return heap_file_insert(hf, data, length);
}

int heap_file_read(HeapFile* hf, TID tid, void* out, uint16_t length) {

    logger(from, "Starting heap_file_read()...");

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    logger(from, "Reading page from disk.");

    Page page;
    disk_read_page(hf->dm, page_id, &page);

    logger(from, "Reading record from page.");

    int result = page_read(&page, slot_id, out, length);

    logger(from, "heap_file_read completed successfully.");

    return result;
}

void heap_file_delete(HeapFile* hf, TID tid) {

    logger(from, "Starting heap_file_delete()...");

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    logger(from, "Reading page from disk.");

    Page page;
    disk_read_page(hf->dm, page_id, &page);

    logger(from, "Deleting record from page.");

    page_delete(&page, slot_id);

    logger(from, "Adding page back to the free page list.");

    page.header.next_free_page = hf->first_free_page;
    hf->first_free_page = page_id;

    logger(from, "Writing updated page back to disk.");

    disk_write_page(hf->dm, page_id, &page);

    logger(from, "heap_file_delete completed successfully.");
}

TID heap_file_update(HeapFile* hf, TID tid, void* data, uint16_t length) {

    logger(from, "Starting heap_file_update()...");

    SlotId slot_id = tid.slot_id;
    PageId page_id = tid.page_id;

    Page page;
    disk_read_page(hf->dm, page_id, &page);

    SlotId post_update_slot_id  = page_update(&page , slot_id, data, length);

    if (post_update_slot_id != INVALID_SLOT_ID) {
        logger(from ,"Update complete successfully");

        disk_write_page(hf->dm , page_id, &page);

        logger(from, "heap_file_update completed successfully.");

        return (TID){.slot_id = post_update_slot_id , .page_id = page_id};
    }

    logger(from, "Starting OUT-OF-PAGE-RELOCATION");

    logger(from , "Deleting old record from this page");
    heap_file_delete(hf , tid);


    logger(from , "Inserting new record in another page...");
    TID newTid = heap_file_insert(hf , data, length);


    logger(from , "Page Successfully Updated");
    return newTid;
}


