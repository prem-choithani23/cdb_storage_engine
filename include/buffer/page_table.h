//
// Created by prem-choithani on 8/21/26.
//

#ifndef STORAGE_ENGINE_PAGE_TABLE_H
#define STORAGE_ENGINE_PAGE_TABLE_H
#include <stdbool.h>

#include "../common/config.h"
#include "page_table_node.h"


typedef struct page_table {
    int capacity;
    int size;
    PageTableNode ** buckets;
}PageTable;

static inline unsigned long hash(const PageId page_id, size_t capacity) {
    return (page_id * 2654435761u) % capacity;
}

PageTable * page_table_create(int capacity);
bool page_table_insert(PageTable * page_table , PageId page_id, FrameId frame_id);
FrameId page_table_lookup(PageTable * page_table , PageId page_id);
bool page_table_delete(PageTable * page_table , PageId page_id);
void page_table_free(PageTable * page_table);

#endif //STORAGE_ENGINE_PAGE_TABLE_H
