//
// Created by prem-choithani on 8/21/26.
//

#ifndef STORAGE_ENGINE_PAGE_TABLE_ENTRY_H
#define STORAGE_ENGINE_PAGE_TABLE_ENTRY_H

#include "../common/config.h"

typedef struct pte_node {
    PageId page_id;
    FrameId frame_idx;
    struct pte_node * next;
}PageTableNode;

#endif //STORAGE_ENGINE_PAGE_TABLE_ENTRY_H
