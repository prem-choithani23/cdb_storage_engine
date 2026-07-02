//
// Created by prem-choithani on 7/1/26.
//


#include <stdint.h>
#include "../headers/table_api.h"

#include <stdlib.h>

#include "../headers/logger.h"

static const char * from = "TABLE_API";

TableAPI  * table_api_open(const char* path) {
    logger(from, "Opening Table API");

    TableAPI * table_api = (TableAPI *) malloc(sizeof(TableAPI));
    table_api->hf = heap_file_open(path);

    logger(from, "Table API opened successfully");
    return table_api;
}

void table_api_close(TableAPI * api) {
    heap_file_close(api->hf);
    free(api);
}

TID insert_data(TableAPI * api , void * data , uint16_t length) {

    logger(from, "Starting insertion...");

    TID tid = heap_file_insert(api->hf , data, length);

    logger(from, "Insertion successful");

    return tid;
}

int read_data(TableAPI * api , TID tid , void *out  , uint16_t length) {

    logger(from, "Starting reading...");

    int result = heap_file_read(api->hf  , tid ,  out , length);

    logger(from, "Reading successful");

    return result;
}

TID update_data(TableAPI * api , TID tid , void * data , uint16_t length) {

    logger(from, "Starting updating...");

    TID post_update_tid = heap_file_update(api->hf, tid , data , length);

    logger(from, "Updating successful");

    return post_update_tid;
}

void delete_data(TableAPI * api, TID tid) {

    logger(from, "Starting deleting...");

    heap_file_delete(api->hf  , tid);

    logger(from, "Deleting successful");
}