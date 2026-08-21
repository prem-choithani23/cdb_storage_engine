//
// Created by prem-choithani on 7/1/26.
//
#include <stdint.h>
#include "../headers/slotted_page.h"

#include <string.h>

#include "../../include/storage/slot.h"
#include "../../include/logging/logger.h"

static const char * from  = "SLOTTED_PAGE";

SlotId page_insert(Page * page , void * data ,  uint16_t length) {

    logger(from , "Initializing page_insert...");

    Slot slot;

    int valid_slot_insert_offset = page->header.slot_count * sizeof(Slot);

    int freeSpaceOffset = page->header.free_space_pointer;

    size_t free_space = get_free_space(page);

    logger(from , "Calculated slot position and available free space.");

    if (free_space < length + sizeof(Slot))
    {
        logger(from , "Cannot insert data in this page - NoSpaceException");
        return INVALID_SLOT_ID;
    }
    slot.offset = freeSpaceOffset - length;
    slot.length = length;

    page->header.free_space_pointer = slot.offset;
    int slotId = page->header.slot_count;
    page->header.slot_count++;
    logger(from , "Writing slot metadata and record payload.");

    memcpy(&page->data[valid_slot_insert_offset], &slot, sizeof(Slot));
    memcpy(&page->data[slot.offset], data, length);

    logger(from , "Record inserted successfully.");

    return slotId;

}

int page_read(Page* page, SlotId slot_id, void* out, uint16_t length) {

    logger(from , "Initializing page_read...");

    PageHeader header = page->header;

    if ( header.slot_count <= slot_id ) {
        logger(from , "Not enough slots in this page.");
        return 0;
    }

    int slot_offset = slot_id * sizeof(Slot);

    Slot slot;
    memcpy(&slot , &page->data[slot_offset] , sizeof (Slot));

    int record_offset  = slot.offset;
    int record_length = slot.length;

    if (record_length == 0) {
        logger(from, "Slot is tombstoned - record deleted");
        return 0;
    }

    if ( record_length > length ) {
        logger(from , "Actual record length is greater than record length - OutputBufferOverflowException");
        return 0;

    }
    memcpy(out , &page->data[record_offset] , record_length);

    logger(from , "Record read successfully.");

    return 1;
}

SlotId page_update(Page* page, SlotId slot_id, void* data, uint16_t length) {
    logger(from , "Initializing page_update...");
    PageHeader header = page->header;

    if (header.slot_count <= slot_id) {
        logger(from , "Not enough slots in this page.");
    }

    int slot_offset = slot_id * sizeof(Slot);
    Slot slot;
    memcpy(&slot , &page->data[slot_offset] , sizeof (Slot));
    int record_offset = slot.offset;
    int record_length = slot.length;


    if (record_length == 0) {
        logger(from , "Slot is tombstoned - record deleted");
        return INVALID_SLOT_ID;
    }

    if ( length <= record_length ) {
        logger(from , "New record size LESS THAN old record --> In Place Update");
        slot.length = length;
        memcpy(&page->data[slot_offset], &slot, sizeof(Slot));
        memcpy(&page->data[record_offset], data, length);
        logger(from , "Record updated successfully.");
        return slot_id;
    }

    logger(from , "New record size GREATER THAN old record");

    size_t free_space = get_free_space(page);

    if ( free_space >= length ) {
        logger(from , "Performing insertion of new record & tombstoning of old record");

        size_t free_space_offset = header.free_space_pointer;
        int new_record_offset = free_space_offset - length;

        slot.length = length;
        memcpy(&page->data[new_record_offset] , data , length);
        memcpy(&page->data[slot_offset]  , &slot, sizeof(Slot));

        slot.offset = new_record_offset;
        page->header.free_space_pointer = slot.offset;

        logger(from , "New Record inserted successfully.");
        return slot_id;

    }

    logger(from , "Not enough free space to place the new record in this page.");
    return INVALID_SLOT_ID;
}

void page_delete(Page * page , SlotId slot_id) {
    logger(from , "Initializing page_delete...");

    PageHeader header = page->header;

    if ( header.slot_count <= slot_id ) {
        logger(from , "Not enough slots in this page.");
        return;
    }

    int slot_offset = slot_id * sizeof(Slot);

    Slot slot;
    memcpy(&slot , &page->data[slot_offset] , sizeof (Slot));

    // mark the record as tombstone ( set slot.length = 0)
    slot.length = 0;

    // write the slot again
    memcpy(&page->data[slot_offset], &slot, sizeof(Slot));
    logger(from , "Set the slot length to zero to mark record as TOMBSTONE");


    logger(from , "Record deleted successfully.");
}