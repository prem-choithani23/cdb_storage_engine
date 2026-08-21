//
// Created by prem-choithani on 7/1/26.
//

#ifndef STORAGE_ENGINE_TABLE_API_H
#define STORAGE_ENGINE_TABLE_API_H
#include "../../headers/heap_file.h"


typedef struct {
    HeapFile * hf;
} TableAPI;

TableAPI  * table_api_open(const char* path);
void table_api_close(TableAPI * table);

TID insert_data(TableAPI * api , void * data , uint16_t length);
int read_data(TableAPI * api , TID tid , void *out , uint16_t length);
TID update_data(TableAPI * api , TID tid , void * data , uint16_t length);
void delete_data(TableAPI * api, TID tid);
#endif //STORAGE_ENGINE_TABLE_API_H
