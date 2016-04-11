//
// Created by David Tenty on 2016-04-07.
//

#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "logging.h"
#include "fatoperations.h"
#include "diskstructures.h"

pthread_mutex_t fatmutex = PTHREAD_MUTEX_INITIALIZER;
blockptr *fat;
uint32_t freeblocks;

void writefat();

void fatinit() {
    freeblocks=0;
    for (int i=0; i < super.numblocks; i++) {
        if (fat[i]==DAV_UNALLOCATED)
            freeblocks++;
    }
}

blockptr fatlookup(const blockptr entry) {
    return fat[entry];
}

int fatextendblocks(blockptr entry, blockptr *newblock) {
    if (freeblocks==0)
        return -ENOSPC;
    blockptr i;
    pthread_mutex_lock(&fatmutex);
    for (i=0; i < super.numblocks; i ++) {
        if (fat[i]==DAV_UNALLOCATED) {
            fat[entry]=i;
            fat[i]=DAV_EOF;
            break;
        }
    }
    writefat();
    freeblocks--;
    pthread_mutex_unlock(&fatmutex);
    davfslognum("Extended chain to include block ", i);
    *newblock=i;
    return 0;
}

/*
 * The caller should be holding the fat mutex when invoking this procedure
 */
void writefat() {
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice,OFFSET_FAT*BLOCKSIZE,SEEK_SET);
    write(blockdevice,fat,fatsize*BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
}

int fatnewchain(blockptr *newchain) {
    if (freeblocks==0)
        return -ENOSPC;
    blockptr i;
    pthread_mutex_lock(&fatmutex);
    for (i=0; i < super.numblocks; i ++) {
        if (fat[i]==DAV_UNALLOCATED) {
            fat[i]=DAV_EOF;
            break;
        }
    }
    writefat();
    freeblocks--;
    pthread_mutex_unlock(&fatmutex);
    davfslognum("Creating chain on block ", i);
    *newchain=i;
    return 0;
}

int getblock(blockptr start, int n, blockptr *end, int extend) {
    int ret;
    blockptr next=start,previous;
    if (n==0) {
        *end=next;
        return 0;
    }
    for (int i=0; i < n; i++) {
        previous=next;
        next=fatlookup(next);
        if (next==DAV_EOF) {
            if (extend) {
                ret = fatextendblocks(previous, &next);
                if (ret < 0)
                    return ret;
            } else {
                return -EINVAL;
            }
        }
    }
    assert(next>fatsize); // we shouldn't be returning a reserved block
    *end=next;
    return 0;
}

void freechain(blockptr start) {
    blockptr current;
    blockptr next=start;
    pthread_mutex_lock(&fatmutex);
    do {
        current=next;
        next=fatlookup(current);
        fat[current]=DAV_UNALLOCATED;
        freeblocks++;
    } while (next != DAV_EOF);
    writefat();
    pthread_mutex_unlock(&fatmutex);
}