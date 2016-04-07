//
// Created by David Tenty on 2016-04-05.
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "diskstructures.h"

void initsuperblock(superblock *super, uint64_t blockcount) {
    strcpy(super->magic,"DAVFS");
    super->numblocks=blockcount;

    assert(sizeof(superblock)==BLOCKSIZE);
    assert(sizeof(dirent)*2==BLOCKSIZE);
}

int64_t fatSize(const superblock *super) {// create file allocation table
    int64_t fatsize=super->numblocks*sizeof(blockptr) / BLOCKSIZE;

    if ((super->numblocks*sizeof(blockptr) % BLOCKSIZE) != 0) // round to the nearest whole block
        fatsize++;
    return fatsize;
}