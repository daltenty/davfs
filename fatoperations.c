//
// Created by David Tenty on 2016-04-07.
//

#include <unistd.h>
#include "logging.h"
#include "fatoperations.h"
#include "diskstructures.h"

pthread_mutex_t fatmutex = PTHREAD_MUTEX_INITIALIZER;
blockptr *fat;

blockptr fatlookup(const blockptr entry) {
    return fat[entry];
}

blockptr fatextendblocks(const blockptr entry) {
    blockptr i;
    pthread_mutex_lock(&fatmutex);
    for (i=0; i < super.numblocks; i ++) {
        if (fat[i]==DAV_UNALLOCATED) {
            fat[entry]=i;
            fat[i]=DAV_EOF;
            break;
        }
    }
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice,OFFSET_FAT,SEEK_SET);
    write(blockdevice,fat,fatsize*BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
    pthread_mutex_unlock(&fatmutex);
    davfslognum("Extended chain to include block ", i);
    return i;
}

blockptr fatnewchain() {
    blockptr i;
    pthread_mutex_lock(&fatmutex);
    for (i=0; i < super.numblocks; i ++) {
        if (fat[i]==DAV_UNALLOCATED) {
            fat[i]=DAV_EOF;
            break;
        }
    }
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice,OFFSET_FAT,SEEK_SET);
    write(blockdevice,fat,fatsize*BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
    pthread_mutex_unlock(&fatmutex);
    davfslognum("Creating chain on block ", i);
    return i;
}

blockptr getblock(blockptr start, int n) {
    blockptr ret=start;
    if (n==0)
        return ret;
    for (int i=0; i < n; i++) {
        ret=fatlookup(ret);
    }
    return ret;
}