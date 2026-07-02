//
// Created by prem-choithani on 7/1/26.
//

#ifndef STORAGE_ENGINE_SLOTTED_PAGE_H
#define STORAGE_ENGINE_SLOTTED_PAGE_H
#include <stdint.h>
#include "page.h"
#include "config.h"


SlotId page_insert(Page * page , void * data ,  uint16_t length);
int page_read(Page* page, SlotId slot_id, void* out, uint16_t length);
SlotId page_update(Page* page, SlotId slot_id, void* data, uint16_t length);
void page_delete(Page * page , SlotId slot_id);

#endif //STORAGE_ENGINE_SLOTTED_PAGE_H
