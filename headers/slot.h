//
// Created by prem-choithani on 7/1/26.
//

#ifndef STORAGE_ENGINE_SLOT_H
#define STORAGE_ENGINE_SLOT_H
#include <stdint.h>

typedef struct {
    uint16_t offset;
    uint16_t length;
} Slot;
#endif //STORAGE_ENGINE_SLOT_H
