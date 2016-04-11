#include "fsoperations.h"
#include "diskstructures.h"
#include "logging.h"
#include "fatoperations.h"


#define LOGFILE "/tmp/davfs"
#define PERM_MODE 0777


//pthread_mutex_t handlesmutex = PTHREAD_MUTEX_INITIALIZER;




/*
blockptr filehandles[MAXHANDLES];
uint32_t nexthandle=0;

uint32_t gethandle() {
    uint32_t handle;
    pthread_mutex_lock(&handlesmutex);
    handle=nexthandle;
    nexthandle++;
    nexthandle=nexthandle%MAXHANDLES;
    pthread_mutex_unlock(&handlesmutex);
   return handle;
}*/


static int davfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {
    if (key == FUSE_OPT_KEY_NONOPT && devicepath == NULL) {
        devicepath = strdup(arg);
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    devicepath=NULL;

    // parse program options
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    fuse_opt_parse(&args, NULL, NULL, davfs_opt_proc);

    // open logfile
    logfile=fopen(LOGFILE,"w");

    // check device
    struct stat device;

    if (stat(devicepath,&device) == -1) {
        perror("Error stating device");
        exit(1);
    }
    davfs_mount(open(devicepath,O_RDWR));

    umask(0);
    fuse_main(args.argc, args.argv, &davfsops, NULL);
    davfslog("DAVFS Stopping");
    close(blockdevice);
    fclose(logfile);
    davfs_unmount();
    return 0;
}

