//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_FATOPERATIONS_H
#define DAVFS_FATOPERATIONS_H

#include "globals.h"

blockptr fatlookup(const blockptr entry);
int fatextendblocks(blockptr entry, blockptr *newblock);
int fatnewchain(blockptr *newchain);
int getblock(blockptr start, int n, blockptr *end, int extend);
void fatinit();
void freechain(blockptr start);

#endif //DAVFS_FATOPERATIONS_H
