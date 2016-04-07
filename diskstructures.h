//
// Created by David Tenty on 2016-04-05.
//

#ifndef DAVFS_DISKSTRUCTURES_H
#define DAVFS_DISKSTRUCTURES_H

#include <stdint.h>

#define BLOCKSIZE 512
#define NAMELEN 247

typedef uint32_t blockptr;

#define DAV_UNALLOCATED 0x00000000
#define DAV_EOF 0x00000001
#define DAV_SPECIAL 0xFFFFFFFF

#define DAV_INVALID 0x00
#define DAV_DIR 0x01
#define DAV_FILE 0x02

#define OFFSET_SUPERBLOCK 0
#define OFFSET_FAT 1



typedef struct  {
    char magic[8]; // "DAVFS"
    uint64_t numblocks;
    uint64_t padding[62];
} superblock; // 512 bytes

typedef struct {
    char name[NAMELEN];
    uint8_t type;
    uint32_t size;
    blockptr ptr;
} dirent;

typedef struct {
    dirent entries[2];
} dirpair;

void initsuperblock(superblock *super, uint64_t blockcount);
int64_t fatSize(const superblock *super);


#endif //DAVFS_DISKSTRUCTURES_H
