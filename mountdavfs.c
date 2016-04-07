#define FUSE_USE_VERSION 30

#ifdef ___APPLE__
#define _DARWIN_FEATURE_64_BIT_INODE
#endif

#ifdef __linux__
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#define _BSD_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fuse.h>
#include <unistd.h>
#include <sys/mount.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <libgen.h>
#include <assert.h>

#include "diskstructures.h"
#include "logging.h"
#include "fatoperations.h"

#define LOGFILE "/tmp/davfs"
#define PERM_MODE 0777

char *devicepath;
int blockdevice;
superblock super;
pthread_mutex_t diskmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;
FILE *logfile;
int64_t fatsize;
dirent rootdir;


/*
 * Given a directory and name, search for the entry in the directory
 *
 * /param name the name to search for
 * /param dir the directory to search
 * /param result the dirent to fill with results
 * /return zero if successful, -errno elsewise
 */
int findInDirectory(const char *name, const dirent *dir, dirent *result) {
    assert(strcmp(name,"")!=0); // filenames cannot be blank
    davfslogstr("Searching for entry ", name);
    dirpair pair;


    blockptr searchblock=dir->ptr;
    do {
        //load the directory pair
        pthread_mutex_lock(&diskmutex);
        lseek(blockdevice, BLOCKSIZE * searchblock, SEEK_SET);
        read(blockdevice, &pair, BLOCKSIZE);
        pthread_mutex_unlock(&diskmutex);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type != DAV_INVALID && strcmp(pair.entries[i].name, name) == 0) {
                memcpy(result, pair.entries + i, sizeof(dirent));
                return 0;
            }
        }

        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    return -ENOENT;
}

/*
 * Given a partial path search for the directory entry
 * relative to base
 *
 * /param path the partial path to search for
 * /param base the dir to search
 * /param dir the dirent to fill with results
 * /return zero if successful, -errno elsewise
 */
int traversepath(const char *path, const dirent *base, dirent *dir) {
    //davfslogstr("Traversing path ", path);
    dirent basedir;

    // check if we are searching for the root dir
    if (strcmp(path,"/")==0) {
        assert(base==&rootdir);
        memcpy(dir, base, sizeof(dirent));
        return 0;
    }

    // slice the path into head and tail
    char *headbuff=strdup(path);

    char *head=headbuff+1;
    char *tail=NULL;

    char *sep=index(head, '/');
    if (sep!=NULL) {
        *sep = '\0';
         tail= index(path + 1, '/');
    }

    int ret = findInDirectory(head, base, &basedir);
    free(headbuff);
    if (ret < 0) {
        return ret; // not found
    }
    else if (tail!=NULL) {
        return traversepath(tail, &basedir, dir); // recurse on tail
    }
    else {
        memcpy(dir,&basedir, sizeof(dirent)); // found
        return 0;
    }
}

static int davfs_getattr(const char *path, struct stat *stbuf) {
    davfslogstr("Checking attributes on ", path);
    memset(stbuf,0,sizeof(struct stat));

    // lookup the directory entry
    dirent entry;
    int exit=traversepath(path,&rootdir,&entry);
    if (exit < 0)
        return exit;

    // populate the stat structure
    stbuf->st_mode = entry.type==DAV_DIR ? (S_IFDIR | PERM_MODE) : (S_IFREG | PERM_MODE);
    stbuf->st_nlink = 2;

    return 0;
}

static int davfs_mkdir(const char *path, mode_t mode) {
    davfslogstr("Creating directory ", path);
    dirpair zeroblock;
    memset(&zeroblock,0, sizeof(dirpair));
    dirent basedir,newdir;

    char *buff=strdup(path);
    char *dirn=dirname(buff);

    // look up root dir
    int ret=traversepath(dirn,&rootdir,&basedir);
    if (ret < 0)
        return ret;

    // get and zero new directory block
    newdir.ptr= fatnewchain();
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * newdir.ptr, SEEK_SET);
    write(blockdevice, &zeroblock, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);

    newdir.type=DAV_DIR;
    strcpy(newdir.name,basename(buff));
    free(buff);

    davfslogstr("Dirname: ", newdir.name);

    dirpair pair;

    blockptr previousblock;
    blockptr searchblock=basedir.ptr;
    do {
        davfslognum("Search block: ", searchblock);
        //load the directory pair
        pthread_mutex_lock(&diskmutex);
        lseek(blockdevice, BLOCKSIZE * searchblock, SEEK_SET);
        read(blockdevice, &pair, BLOCKSIZE);
        pthread_mutex_unlock(&diskmutex);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type == DAV_INVALID) {
                memcpy(pair.entries + i, &newdir, sizeof(dirent));
                pthread_mutex_lock(&diskmutex);
                lseek(blockdevice, BLOCKSIZE * searchblock, SEEK_SET);
                write(blockdevice, &pair, BLOCKSIZE);
                pthread_mutex_unlock(&diskmutex);

                return 0;
            }
        }

        previousblock=searchblock;
        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    searchblock = fatextendblocks(previousblock);
    dirpair newpair;
    memcpy(newpair.entries, &newdir, sizeof(dirent));
    pthread_mutex_lock(&diskmutex);
    lseek(blockdevice, BLOCKSIZE * searchblock, SEEK_SET);
    write(blockdevice, &newpair, BLOCKSIZE);
    pthread_mutex_unlock(&diskmutex);

    return 0;
}

static int davfs_access(const char *path, int mask) {
    //davfslogstr("Checking access on ", path);

    dirent dir;
    return traversepath(path,&rootdir,&dir);
}

static int davfs_readlink(const char *path, char *buf, size_t size) {
    davfslogstr("Reading link ", path);
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
    davfslogstr("Reading ", path);
    return -EINVAL;
}

static int davfs_open(const char *path, struct fuse_file_info *fi) {
    davfslogstr("Opening ", path);
    return -EACCES;
}

static int davfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
    davfslogstr("Reading directory: ", path);
    dirent dir;

    int ret=traversepath(path,&rootdir,&dir);
    if (ret < 0)
        return ret;

    dirpair pair;

    blockptr searchblock=dir.ptr;
    if (offset==0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
    }

    do {
        //load the directory pair
        pthread_mutex_lock(&diskmutex);
        lseek(blockdevice, BLOCKSIZE * searchblock, SEEK_SET);
        read(blockdevice, &pair, BLOCKSIZE);
        pthread_mutex_unlock(&diskmutex);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type != DAV_INVALID) {
                struct stat sbuff;
                sbuff.st_mode= pair.entries[i].type==DAV_DIR ? (S_IFDIR | PERM_MODE) : (S_IFREG | PERM_MODE);
                filler(buf,pair.entries[i].name,&sbuff,0);
            }
        }

        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    return 0;
}

static int davfs_statfs(const char *path, struct statvfs *stbuf) {
    davfslog("Statfs called");
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
        .access		= davfs_access,
        .readlink	= davfs_readlink,
        .readdir	= davfs_readdir,
//        .mknod		= davfs_mknod,
        .mkdir		= davfs_mkdir,
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
        fprintf(stderr,"Invalid filesystem header\n");
        exit(1);
    } else {
        printf("Good davfs signature\n");
        printf("Block count: %ld\n", super.numblocks);
    }

    davfslog("DAVFS Starting");

    // calculate fat size
    fatsize = fatSize(&super);

    //load the FAT into memory
    fat=(blockptr*)malloc(BLOCKSIZE*fatsize);
    read(blockdevice,fat,BLOCKSIZE*fatsize);

    // build root directory
    strcpy(rootdir.name,"/");
    rootdir.ptr=OFFSET_FAT+fatsize;
    rootdir.type=DAV_DIR;

    umask(0);
    fuse_main(args.argc, args.argv, &davfsops, NULL);
    davfslog("DAVFS Stopping");
    close(blockdevice);
    fclose(logfile);
    free(fat);
    return 0;
}