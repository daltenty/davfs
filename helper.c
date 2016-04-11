//
// Created by David Tenty on 2016-04-05.
//

#include <sys/disk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
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

int davfs_initialize(int disk, uint32_t numblocks) {
    char zeroblock[BLOCKSIZE];
    memset(zeroblock,0,BLOCKSIZE);

    // create superblock
    superblock *super=(superblock*)malloc(sizeof(superblock));
    initsuperblock(super,numblocks);

    if (write(disk, super, sizeof(superblock)) != sizeof(superblock)) {
        return 1;
    }
    int64_t fatsize = fatSize(super);

    //printf("FAT requires: %ld blocks\n", fatsize);
    blockptr *fat=malloc(fatsize*BLOCKSIZE);
    for (int i=0; i < OFFSET_FAT+fatsize; i++) {
        fat[i]=DAV_SPECIAL;
        //printf("reserving block: %d\n",i);
    }
    fat[OFFSET_FAT+fatsize]=DAV_EOF; // allocate the root directory
    write(disk,fat,BLOCKSIZE*fatsize);


    // allocate root directory
    write(disk,zeroblock,BLOCKSIZE);

    return 0;
}