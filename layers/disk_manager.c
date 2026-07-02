//
// Created by prem-choithani on 6/7/26.
//

#define _XOPEN_SOURCE 700


#include <stdint.h>

#include "../headers/config.h"
#include "../headers/disk_manager.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../headers/logger.h"
#include "../headers/page.h"


static const char * from  = "DISK_MANAGER";

DiskManager * disk_manager_open(const char * path) {

    logger(from , "Opening Disk manager");
    DiskManager * disk_manager = (DiskManager *)malloc(sizeof(DiskManager));
    disk_manager->fd = open(path, O_RDWR | O_CREAT);

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
    }

    const off_t file_size = file_info.st_size;

    const PageId newPageId = file_size/PAGE_SIZE;

    Page page;
    memset(&page, 0, sizeof(Page));          // zero everything first
    page.header.slot_count = 0;              // then set fields
    page.header.free_space_pointer = PAGE_PAYLOAD_SIZE;
    page.header.page_id = newPageId;
    page.header.next_free_page = INVALID_PAGE_ID;

    pwrite(disk_manager->fd, &page, PAGE_SIZE, (off_t)newPageId * PAGE_SIZE);

    return newPageId;
}

void disk_deallocate_page(DiskManager * disk_manager , PageId pid) {
    // not yet

    (void)disk_manager;
    (void)pid;
}
