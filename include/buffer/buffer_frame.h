//
// Created by prem-choithani on 8/21/26.
//

#ifndef STORAGE_ENGINE_BUFFER_FRAME_H
#define STORAGE_ENGINE_BUFFER_FRAME_H
#include "../common/config.h"
#include <stdbool.h>

#include "../storage/page.h"

typedef struct buffer_frame {
    Page page;
    int pin_count;
    PageId page_id;
    bool is_occupied;
    bool is_dirty;
    bool ref_bit;
}BufferFrame;
#endif //STORAGE_ENGINE_BUFFER_FRAME_H
