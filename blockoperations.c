//
// Created by David Tenty on 2016-04-07.
//

#include <unistd.h>
#include <assert.h>
#include "blockoperations.h"
#include "logging.h"

pthread_mutex_t diskmutex = PTHREAD_MUTEX_INITIALIZER;

void writeblock(const void *data, blockptr location) {
    float timestamp=time(NULL);
    fprintf(tracefile,"%f\t%d\t%d\t%d\t%d\n",timestamp,0,location,1,0);
    fflush(tracefile);
    davfslognum("Writing block: ",location);
    assert(location>fatsize); // don't overwrite the superblock or fat
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * location, SEEK_SET);
    write(blockdevice, data, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
}

void readblock(void *data, blockptr block) {
    float timestamp=time(NULL);
    fprintf(tracefile,"%f\t%d\t%d\t%d\t%d\n",timestamp,0,block,1,1);
    fflush(tracefile);
    davfslognum("Reading block: ",block);
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * block, SEEK_SET);
    read(blockdevice, data, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);
}
