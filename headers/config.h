//
// Created by prem-choithani on 7/1/26.
//

#include <sys/types.h>
#include "slot.h"
#ifndef STORAGE_ENGINE_CONSTANTS_H
#define STORAGE_ENGINE_CONSTANTS_H

#define PAGE_SIZE 4096

#define DATABASE_FILE_PATH "database.db" // wrt main.c

#define FIRST_FIT 0
#define BEST_FIT 1
#define WORST_FIT 2
#define NEXT_FIT 3

#define INVALID_SLOT_ID 0xFF
#define INVALID_PAGE_ID UINT16_MAX

#define ENABLE_LOGGING 0

typedef u_int32_t PageId;
typedef u_int8_t SlotId;

#define MIN_FREE_SPACE (sizeof(Slot) + 1)

#endif //STORAGE_ENGINE_CONSTANTS_H
