//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_LOGGING_H
#define DAVFS_LOGGING_H

#include "globals.h"

void davfslog(const char *string);
void davfslogstr(const char *string1,const char *string2);
void davfslognum(const char *string1,uint64_t num);

#endif //DAVFS_LOGGING_H
