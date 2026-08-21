//
// Created by prem-choithani on 7/1/26.
//

#include "../../include/storage/page.h"
#include "../../include/storage/slot.h"

size_t get_free_space(Page * page) {
     PageHeader header = page->header;

     int last_slot_ends_at = sizeof(PageHeader) + header.slot_count*sizeof(Slot);
     size_t free_space = page->header.free_space_pointer - last_slot_ends_at;

     return  free_space;
}
