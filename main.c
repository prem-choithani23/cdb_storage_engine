//
// Created by prem-choithani on 6/7/26.
//

#include "headers/constants.h"
#include "headers/disk_manager.h"

int main(int argc, char *argv[]) {

    DiskManager * disk_manager = disk_manager_open(DATABASE_FILE_PATH);


    // layer calling (will be implemented)


    disk_manager_close(disk_manager);
}
