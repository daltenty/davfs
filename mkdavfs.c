//
// Created by David Tenty
//

#ifdef ___APPLE__
#define _DARWIN_FEATURE_64_BIT_INODE
#endif

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "diskstructures.h"


void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [device or filename]\n", argv[0]);
}

int main(int argc,char *argv[]) {
    superblock *super=(superblock*)malloc(sizeof(superblock));
    char zeroblock[BLOCKSIZE];

    memset(zeroblock,0,BLOCKSIZE);
    // see if we have the device/filename
    if (argc != 2) {
        usage(argv);
        return 1;
    }

    // check the properties to determine if file or blockdevice
    struct stat diskstat;
    stat(argv[1],&diskstat);

    printf("%s: creating filesystem on %s\n",argv[0], argv[1]);
    printf("blocksize: %d bytes\n", BLOCKSIZE);
    printf("count: %ld blocks\n", diskstat.st_blocks);

    // create superblock
    initsuperblock(super,diskstat.st_blocks);

    int disk=open(argv[1],O_WRONLY,S_IRWXU);

    if (disk == -1) {
        perror("Error opening disk");
        return 1;
    }

    if (write(disk,super, sizeof(superblock)) != sizeof(superblock)) {
        perror("Error writing superblock");
    }

    // create file allocation table
    int64_t fatsize=diskstat.st_blocks*sizeof(blockptr) / BLOCKSIZE;

    if ((diskstat.st_blocks*sizeof(blockptr) % BLOCKSIZE) != 0) // round to the nearest whole block
        fatsize++;

    printf("FAT requires: %ld blocks\n", fatsize);
    for (int i=0; i < fatsize; i++) {
        write(disk,zeroblock,BLOCKSIZE);
    }

    // allocate root directory
    write(disk,zeroblock,BLOCKSIZE);


    close(disk);
    return 0;
}