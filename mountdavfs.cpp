#define FUSE_USE_VERSION 30

#include <iostream>
#include <fuse.h>
#include <unistd.h>
#include <sys/mount.h>

#include "diskstructures.h"

#define LOGFILE "/tmp/davfs"

using namespace std;

char *devicepath;
int blockdevice;
superblock super;
pthread_mutex_t diskmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;
FILE *logfile;


void log(const char *string) {
    time_t now=time(0);
    tm *time=localtime(&now);
    pthread_mutex_lock(&logmutex);
    fprintf(logfile,"%s: %s\n",asctime(time),string);
    fflush(logfile);
    pthread_mutex_unlock(&logmutex);
}

void log(const char *string1,const char *string2) {
    time_t now=time(0);
    tm *time=localtime(&now);
    pthread_mutex_lock(&logmutex);
    fprintf(logfile,"%s: %s%s\n",asctime(time),string1,string2);
    fflush(logfile);
    pthread_mutex_unlock(&logmutex);
}

int traversepath(const char *path, dirent *dir) {
    log("Traversing path ",path);
    char *pathsegment;
    char *pathbuff=(char*)malloc(strlen(path)+1);
    dirpair pair;
    strcpy(pathbuff,path);

    //load the root directory pair
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE, SEEK_SET);
    read(blockdevice,&pair,BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);

    // parse the rootdir
    pathsegment = strsep(&pathbuff, "/");
    if (*pathsegment!='\0')
        return -EINVAL;

    while ((pathsegment = strsep(&pathbuff, "/")) != NULL) {
        for (int i=0; i < 2; i++) {
            if (pair.entries[i].type != DAV_INVALID && strcmp(pair.entries[i].name, pathsegment) == 0) {
                memcpy(dir,pair.entries+i, sizeof(dirent));
            }
        }
        return -ENOENT;
    }

    free(pathbuff);
    return 0;
}

static int davfs_getattr(const char *path, struct stat *stbuf) {
    log("Checking attributes on ",path);
//    struct stat { /* when _DARWIN_FEATURE_64_BIT_INODE is defined */
//        dev_t           st_dev;           /* ID of device containing file */
//        mode_t          st_mode;          /* Mode of file (see below) */
//        nlink_t         st_nlink;         /* Number of hard links */
//        ino_t           st_ino;           /* File serial number */
//        uid_t           st_uid;           /* User ID of the file */
//        gid_t           st_gid;           /* Group ID of the file */
//        dev_t           st_rdev;          /* Device ID */
//        struct timespec st_atimespec;     /* time of last access */
//        struct timespec st_mtimespec;     /* time of last data modification */
//        struct timespec st_ctimespec;     /* time of last status change */
//        struct timespec st_birthtimespec; /* time of file creation(birth) */
//        off_t           st_size;          /* file size, in bytes */
//        blkcnt_t        st_blocks;        /* blocks allocated for file */
//        blksize_t       st_blksize;       /* optimal blocksize for I/O */
//        uint32_t        st_flags;         /* user defined flags for file */
//        uint32_t        st_gen;           /* file generation number */
//        int32_t         st_lspare;        /* RESERVED: DO NOT USE! */
//        int64_t         st_qspare[2];     /* RESERVED: DO NOT USE! */
//    };
    memset(stbuf,0,sizeof(struct stat));
    dirent entry;
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1;
    } else {
        int exit=traversepath(path,&entry);
        if (exit < 0)
            return exit;

        stbuf->st_mode = entry.type==DAV_DIR ? (S_IFDIR | 0755) : (S_IFREG | 0755);
        stbuf->st_nlink = 2;
    }
    return -ENOENT;
}

static int davfs_access(const char *path, int mask) {
    log("Checking access on ",path);
    return 0; // FIXME: path existance
}

static int davfs_readlink(const char *path, char *buf, size_t size) {
    log("Reading link ",path);
    return -EINVAL; // we don't do symbolic links
}

static int davfs_symlink(const char *from, const char *to) {
    return -EACCES; // we don't do symbolic links
}

static int davfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    return -EINVAL; // no special files either
}

static int davfs_chown(const char *path, uid_t uid, gid_t gid) {
    return 0; // silently ignore ownership
}

static int davfs_chmod(const char *path, mode_t mode){
    return 0; // silently ignore permission
}

static int davfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    log("Reading ",path);
    return -EINVAL;
}

static int davfs_open(const char *path, struct fuse_file_info *fi) {
    log("Opening ",path);
    return -EACCES;
}

static int davfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{   log("Reading directory: ", path);
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);


    return 0;
}

static int davfs_statfs(const char *path, struct statvfs *stbuf) {
    log("Statfs called");
    stbuf->f_bsize=BLOCKSIZE*8;
    stbuf->f_bavail=super.numblocks-2; //FIXME: evil
    stbuf->f_blocks=super.numblocks;
    stbuf->f_files=10;
    stbuf->f_ffree=1000;
    stbuf->f_namemax=NAMELEN;
    return 0;

}

static struct fuse_operations davfsops = {
        .getattr	= davfs_getattr,
//        .access		= davfs_access,
        .readlink	= davfs_readlink,
        .readdir	= davfs_readdir,
//        .mknod		= davfs_mknod,
//        .mkdir		= xmp_mkdir,
//        .symlink	= davfs_symlink,
//        .unlink		= xmp_unlink,
//        .rmdir		= xmp_rmdir,
//        .rename		= xmp_rename,
//        .link		= xmp_link,
//        .chmod		= davfs_chmod,
//        .chown		= davfs_chown,
//        .truncate	= xmp_truncate,

        .open		= davfs_open,
        .read		= davfs_read,
//        .write		= xmp_write,
//        .statfs		= davfs_statfs,
//        .release	= xmp_release,
//        .fsync		= xmp_fsync,
};

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

    // open device, read superblock and check headers
    blockdevice=open(devicepath,O_RDWR);
    read(blockdevice,&super, BLOCKSIZE);
    if (strcmp("DAVFS",super.magic)!=0) {
        cerr << "Invalid filesystem header" << endl;
    } else {
        cout << "Good davfs signature" << endl;
        cout << "Block count: " << super.numblocks << endl;
    }

    log("DAVFS Starting");


    umask(0);
    fuse_main(args.argc, args.argv, &davfsops, NULL);
    log("DAVFS Stopping");
    close(blockdevice);
    return 0;
}