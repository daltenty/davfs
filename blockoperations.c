//
// Created by David Tenty on 2016-04-07.
//

#include <unistd.h>
#include <assert.h>
#include "blockoperations.h"

pthread_mutex_t diskmutex = PTHREAD_MUTEX_INITIALIZER;

void writeblock(const void *data, blockptr location) {
    assert(location>fatsize); // don't overwrite the superblock or fat
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * location, SEEK_SET);
    write(blockdevice, data, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
}

void readblock(void *data, blockptr block) {
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * block, SEEK_SET);
    read(blockdevice, data, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
}
