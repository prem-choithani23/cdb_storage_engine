//
// Created by prem-choithani on 6/7/26.
//

#ifndef STORAGE_ENGINE_TID_H
#define STORAGE_ENGINE_TID_H

#include "../storage/page.h"
#include "../storage/slotted_page.h"

typedef struct tid {
    PageId page_id;
    SlotId slot_id;
} TID;

#endif //STORAGE_ENGINE_TID_H