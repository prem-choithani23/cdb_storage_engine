cmake_minimum_required(VERSION 3.20)

project(storage_engine C)

set(CMAKE_C_STANDARD 11)

add_executable(storage_engine
        main.c

        src/disk/disk_manager.c
        src/storage/heap_file.c
        src/storage/table_api.c
        src/logging/logger.c
        src/storage/slotted_page.c
        src/storage/page.c
c