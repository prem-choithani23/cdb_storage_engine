//
// Created by prem-choithani on 6/7/26.
//


#include <stdint.h>

#include "../headers/constants.h"
#include "../headers/disk_manager.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../headers/logger.h"


char * from  = "DISK_MANAGER";

DiskManager * disk_manager_open(const char * path) {
    DiskManager * disk_manager = (DiskManager *)malloc(sizeof(DiskManager));
    disk_manager->fd = open(path, O_RDWR | O_CREAT);

    if (disk_manager->fd == -1) {
        log(from , "Failed to open database file");
    }
    return disk_manager;
}

void disk_manager_close(DiskManager * disk_manager) {
    close(disk_manager->fd);
    free(disk_manager);
}

void disk_read_page(const DiskManager * disk_manager , PageId pid, void *buffer) {

    log(from , "Initializing  disk_read_page ...");

    off_t offset = (off_t)pid * PAGE_SIZE;
    ssize_t n = pread(disk_manager->fd, buffer, PAGE_SIZE, offset);

    if (n != PAGE_SIZE) {
        log(from ,"Error while reading the page...");
        return;
    }

    log(from , "Disk read success");

}

void disk_write_page(DiskManager * disk_manager , PageId pid, const void *buffer) {

    log(from , "Initializing  disk_write_page ...");

    off_t offset = (off_t)pid * PAGE_SIZE;

    ssize_t n = pwrite(disk_manager->fd, buffer, PAGE_SIZE, offset);

    if (n != PAGE_SIZE) {
        log(from ,"Error while writing the page...");
    }

    log(from , "Disk write success");
}

PageId disk_allocate_page(const DiskManager * disk_manager) {

    log(from , "Initializing  disk_allocate_page ...");

    struct stat file_info;

    if (fstat(disk_manager->fd, &file_info) == -1) {
        log(from , "Error getting file status");
    }

    const off_t file_size = file_info.st_size;

    const PageId newPageId = file_size/PAGE_SIZE;

    uint8_t zeroed[PAGE_SIZE] = {0};
    pwrite(disk_manager->fd, zeroed, PAGE_SIZE, (off_t)newPageId * PAGE_SIZE);

    return newPageId;
}

void disk_deallocate_page(DiskManager * dsk_manager , PageId pid) {
    // not yet
}
