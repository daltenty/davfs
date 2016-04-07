//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_GLOBALS_H
#define DAVFS_GLOBALS_H

#include <pthread.h>
#include <stdio.h>
#include "diskstructures.h"


#define LOGFILE "/tmp/davfs"
#define PERM_MODE 0777
#define MAXHANDLES 1024

extern char *devicepath;
extern int blockdevice;
extern superblock super;
extern pthread_mutex_t diskmutex;
extern pthread_mutex_t logmutex;
extern pthread_mutex_t fatmutex;
extern FILE *logfile;
extern int64_t fatsize;
extern dirent rootdir;
extern blockptr *fat;
extern blockptr filehandles[];
extern uint32_t nexthandle;

#endif //DAVFS_GLOBALS_H
