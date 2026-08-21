//
// Created by prem-choithani on 7/1/26.
//

#include <stdio.h>
#include "../../include/logging/logger.h"
#include "../../include/common/config.h"

void logger(const char * from , const char * message){
    if (!ENABLE_LOGGING) return;
    printf("[%s] : %s\n", from , message);
}