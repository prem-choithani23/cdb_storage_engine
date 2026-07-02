//
// Created by prem-choithani on 7/1/26.
//

#include <stdio.h>
#include "../headers/logger.h"
#include "../headers/constants.h"

void logger(const char * from , const char * message){
    if (!ENABLE_LOGGING) return;
    printf("[%s] : %s\n", from , message);
}