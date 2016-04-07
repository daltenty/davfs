//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_BLOCKOPERATIONS_H
#define DAVFS_BLOCKOPERATIONS_H

#include "globals.h"

void writeblock(void *data, blockptr location);
void readblock(void *data, blockptr block);

#endif //DAVFS_BLOCKOPERATIONS_H
