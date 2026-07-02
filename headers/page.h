//
// Created by prem-choithani on 6/7/26.
//

#ifndef STORAGE_ENGINE_PAGR_H
#define STORAGE_ENGINE_PAGR_H
#include <stdint.h>
#include<stddef.h>

#define PAGE_HEADER_SIZE sizeof(PageHeader)
#define PAGE_PAYLOAD_SIZE  (4096 - PAGE_HEADER_SIZE)
#define PAGE_SIZE 4096

typedef struct page Page;

typedef struct header{
    uint32_t page_id;            // Which page is this?
    // uint8_t  page_type;          // Heap page? Index page? Overflow page?
    uint16_t slot_count;         // How many slots are in use?
    uint16_t free_space_pointer; // Where does the next record go?

    uint32_t next_free_page;    // pointer to next page

    // uint8_t  is_dirty;           // Modified in memory, not yet on disk?
    // uint64_t lsn;                // Log sequence number (placeholder for now)
} PageHeader;

typedef struct page {
    PageHeader header;
    uint8_t data[PAGE_PAYLOAD_SIZE];
}Page;

size_t get_free_space(Page * page);

#endif //STORAGE_ENGINE_PAGR_H
