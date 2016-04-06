//
// Created by David Tenty
//

#define _DARWIN_FEATURE_64_BIT_INODE

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "diskstructures.h"

using namespace std;

void usage(char *argv[]) {
    cerr << "Usage: " << argv[0] << " [device or filename]" << endl;
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

    cout << argv[0] << ": creating filesystem on " << argv[1] << endl;
    cout << "blocksize: " << BLOCKSIZE << " bytes"  << endl;
    cout << "count: " << diskstat.st_blocks << " blocks" << endl;

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

    cout << "FAT requires: " << fatsize << " blocks" << endl;
    for (int i=0; i < fatsize; i++) {
        write(disk,zeroblock,BLOCKSIZE);
    }

    // allocate root directory
    write(disk,zeroblock,BLOCKSIZE);


    close(disk);
    return 0;
}