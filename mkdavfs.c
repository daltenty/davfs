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
#include <sys/stat.h>
#include <sys/disk.h>
#include <stdlib.h>
#include "diskstructures.h"
#include "helper.h"

void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [device or filename]\n", argv[0]);
}

int main(int argc, char *argv[]) {
    // see if we have the device/filename
    if (argc != 2) {
        usage(argv);
        return 1;
    }

    // check the properties to determine if file or blockdevice
    struct stat diskstat;
    stat(argv[1],&diskstat);
    uint32_t numblocks;

    int disk=open(argv[1],O_WRONLY,S_IRWXU);

    if (disk == -1) {
        perror("Error opening disk");
        return 1;
    }

    printf("%s: creating filesystem on %s which is a ",argv[0], argv[1]);
    if (diskstat.st_mode & S_IFREG) {
        printf("file\n");
        numblocks=diskstat.st_blocks; // FIXME: fix for variable blocksize
    } else if (diskstat.st_mode & S_IFBLK) {
        printf("blockdevice\n");
        #ifdef __linux__
            ioctl(disk, BLKGETSIZE, &numblocks);
        #else
            ioctl(disk,DKIOCGETBLOCKCOUNT, &numblocks);
        #endif
    } else {
        fprintf(stderr,"Invalid device\n");
        exit(1);
    }

    printf("blocksize: %d bytes\n", BLOCKSIZE);
    printf("count: %d blocks\n", numblocks);

    if (davfs_initialize(disk, numblocks) != 0) {
        perror("Error initalizing disk");
    }

    close(disk);
    return 0;
}

