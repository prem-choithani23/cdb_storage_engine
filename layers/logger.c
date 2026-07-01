//
// Created by prem-choithani on 7/1/26.
//

#include <stdio.h>
#include "../headers/logger.h"

void log(char * from , char * message){
    printf("[%s] : %s\n", from , message);
}