//
// Created by prem-choithani on 8/21/26.
//

#ifndef STORAGE_ENGINE_BUFFER_POOL_H
#define STORAGE_ENGINE_BUFFER_ POOL_H
#include <stdlib.h>
#include <string.h>

#include "../common/config.h"
#include "buffer_frame.h"
#include "page_table.h"
#include "../disk/disk_manager.h"

typedef struct {
    BufferFrame buffer_frames[BUFFER_POOL_SIZE];
    int clock_hand;
    PageTable *page_table;
    DiskManager *dm;
}BufferPool;


BufferPool * buffer_pool_open(const char * path );
void buffer_pool_close(BufferPool * buffer_pool);


Page * buffer_pool_fetch_page(BufferPool * buffer_pool , PageId page_id);
void buffer_pool_unpin_page(BufferPool * buffer_pool , PageId page_id , bool is_dirty);
bool buffer_pool_flush_page(BufferPool * buffer_pool , PageId page_id);
Page * buffer_pool_new_page(BufferPool * buffer_pool , PageId * out_page_id);
FrameId clock_evict(BufferPool * buffer_pool);



#endif //STORAGE_ENGINE_BUFFER_POOL_H


