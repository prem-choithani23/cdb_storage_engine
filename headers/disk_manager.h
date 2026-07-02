//
// Created by prem-choithani on 7/1/26.
//

#ifndef STORAGE_ENGINE_DISK_MANAGER_H
#define STORAGE_ENGINE_DISK_MANAGER_H
#include "../headers/config.h"

typedef struct {
    int fd;
}DiskManager;

DiskManager * disk_manager_open(const char * path);
void disk_manager_close(DiskManager * disk_manager);


void disk_read_page (const DiskManager * dsk_manager , PageId pid, void *buffer);
void disk_write_page(DiskManager * dsk_manager , PageId pid, const void *buffer);
PageId disk_allocate_page(const DiskManager * dsk_manager);
void disk_deallocate_page(DiskManager * dsk_manager , PageId pid);


#endif //STORAGE_ENGINE_DISK_MANAGER_H
