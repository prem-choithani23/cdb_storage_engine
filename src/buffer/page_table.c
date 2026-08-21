//
// Created by prem-choithani on 8/21/26.
//

#include "../../include/buffer/page_table.h"

#include <stdlib.h>


PageTable * page_table_create(int capacity) {

    PageTable * page_table = (PageTable*)malloc(sizeof(PageTable));
    if (page_table == NULL) {
        return NULL;
    }

    page_table->capacity = capacity;
    page_table->size = 0;

    page_table->buckets = (PageTableNode**)calloc(capacity, sizeof(PageTableNode*));
    if (page_table->buckets == NULL) {
        free(page_table); // Clean up previously allocated memory
        return NULL;
    }

    return page_table;
}


bool page_table_insert(PageTable * page_table , PageId page_id, FrameId frame_id) {
    unsigned long index = hash(page_id , page_table->capacity);
    PageTableNode * current = page_table->buckets[index];

    while ( current != NULL ) {
        if (current->page_id == page_id) {
            current->frame_idx = frame_id;
            return true;
        }
        current = current->next;
    }

    PageTableNode * new_node = (PageTableNode*)malloc(sizeof(PageTableNode));
    if ( new_node == NULL ) {
        return false;
    }
    new_node->page_id = page_id;
    new_node->frame_idx = frame_id;
    new_node->next = page_table->buckets[index];
    page_table->buckets[index] = new_node;
    page_table->size++;

    return true;
}


FrameId page_table_lookup(PageTable * page_table , PageId page_id) {
    unsigned long index = hash(page_id , page_table->capacity);

    PageTableNode * current = page_table->buckets[index];


    while ( current != NULL ) {
        if (current->page_id == page_id) {
            return current->frame_idx;
        }
        current = current->next;
    }

    return INVALID_FRAME_ID;
}


bool page_table_delete(PageTable * page_table , PageId page_id) {
    unsigned long index = hash(page_id , page_table->capacity);
    PageTableNode * current = page_table->buckets[index];
    PageTableNode * previous = NULL;

    while ( current != NULL ) {
        if (current->page_id == page_id) {

            if (previous == NULL) {
                page_table->buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }

            free(current);
            page_table->size--;
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}


void page_table_free(PageTable * page_table) {

    for (int i=0; i<page_table->capacity; i++) {
        PageTableNode * current = page_table->buckets[i];
        while ( current != NULL ) {
            PageTableNode * temp = current;
            current = current->next;
            free(temp);
        }
    }

    free(page_table->buckets);
    free(page_table);

    return;
}


