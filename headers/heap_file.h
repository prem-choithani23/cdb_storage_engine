//
// Created by prem-choithani on 6/7/26.
//

#ifndef STORAGE_ENGINE_HEAP_FILE_H
#define STORAGE_ENGINE_HEAP_FILE_H
#include "disk_manager.h"
#include "tid.h"
#include "../headers/slotted_page.h"

typedef struct hf {
    DiskManager * dm;
    PageId first_free_page;
}HeapFile;


HeapFile* heap_file_open(const char* path);
void heap_file_close(HeapFile* hf);


TID heap_file_insert(HeapFile* hf, void* data, uint16_t length);
int heap_file_read(HeapFile* hf, TID tid, void* out, uint16_t length);
TID heap_file_update(HeapFile* hf, TID tid, void* data, uint16_t length);
void heap_file_delete(HeapFile* hf, TID tid);


#endif //STORAGE_ENGINE_HEAP_FILE_H

