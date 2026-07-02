cmake_minimum_required(VERSION 3.20)

project(storage_engine C)

set(CMAKE_C_STANDARD 11)

add_executable(storage_engine
        main.c

        layers/disk_manager.c
        layers/heap_file.c
        layers/table_api.c
        layers/logger.c
        layers/slotted_page.c
        layers/page.c
c