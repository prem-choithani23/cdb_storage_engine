//
// Created by prem-choithani on 6/7/26.
//

// #define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdint.h>

#include "../../include/common/config.h"
#include "../../include/disk/disk_manager.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>
#include "../../include/logging/logger.h"
#include "../../include/storage/page.h"
#include "../../include/storage/slotted_page.h"


static const char * from  = "DISK_MANAGER";

DiskManager * disk_manager_open(const char * path) {

    logger(from , "Opening Disk manager");
    DiskManager * disk_manager = (DiskManager *)malloc(sizeof(DiskManager));
    disk_manager->fd = open(path, O_RDWR | O_CREAT | O_DIRECT, 0644);

    if (disk_manager->fd == -1) {
        logger(from , "Failed to open database file");
        perror("Failed to open file");
    }
    logger(from ,"Disk Manager opened successfully");

    return disk_manager;
}

void disk_manager_close(DiskManager * disk_manager) {
    close(disk_manager->fd);
    free(disk_manager);
}

void disk_read_page(const DiskManager * disk_manager , PageId pid, void *buffer) {

    logger(from , "Initializing  disk_read_page ...");

    off_t offset = (off_t)pid * PAGE_SIZE;
    ssize_t n = pread(disk_manager->fd, buffer, PAGE_SIZE, offset);

    if (n != PAGE_SIZE) {
        logger(from ,"Error while reading the page...");
        return;
    }

    logger(from , "Disk read success");
}

void disk_write_page(DiskManager * disk_manager , PageId pid, const void *buffer) {

    logger(from , "Initializing  disk_write_page ...");

    off_t offset = (off_t)pid * PAGE_SIZE;

    ssize_t n = pwrite(disk_manager->fd, buffer, PAGE_SIZE, offset);

    if (n != PAGE_SIZE) {
        logger(from ,"Error while writing the page...");
    }

    logger(from , "Disk write success");
}

PageId disk_allocate_page(const DiskManager * disk_manager) {

    logger(from , "Initializing  disk_allocate_page ...");

    struct stat file_info;

    if (fstat(disk_manager->fd, &file_info) == -1) {
        logger(from , "Error getting file status");
        perror("fstat failed");
    }

    const off_t file_size = file_info.st_size;
    const PageId newPageId = file_size / PAGE_SIZE;

    logger(from, "Extending database file space implicitly using ftruncate.");

    // Extend the file size tracking implicitly to the new page boundary
    // This creates a sparse block without hitting synchronous disk write penalties
    if (ftruncate(disk_manager->fd, (off_t)(newPageId + 1) * PAGE_SIZE) == -1) {
        logger(from, "Failed structural file extension allocation");
        perror("ftruncate failed");
    }

    logger(from, "New page allocated successfully via virtual sizing.");

    return newPageId;
}



void disk_deallocate_page(DiskManager * disk_manager , PageId pid) {
    // not yet

    (void)disk_manager;
    (void)pid;
}
