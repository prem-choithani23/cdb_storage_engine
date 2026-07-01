cmake_minimum_required(VERSION 4.2)
project(storage_engine C)

set(CMAKE_C_STANDARD 11)

add_library(storage_engine STATIC
        main.c
        headers/heap_file.h
        headers/tid.h
        headers/data/student.h
        headers/page.h
        layers/disk_manager.c
        headers/constants.h
        layers/heap_file.c
        layers/table_api.c
        headers/data/student.h
        layers/logger.c
        headers/disk_manager.h
        headers/logger.h
        headers/slotted_page.h
        layers/slotted_page.c
        headers/slot.h
)
