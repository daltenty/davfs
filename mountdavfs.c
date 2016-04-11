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
#include "blockoperations.h"
#include "directoryoperations.h"

#define LOGFILE "/tmp/davfs"
#define PERM_MODE 0777

char *devicepath;
int blockdevice;
superblock super;

//pthread_mutex_t handlesmutex = PTHREAD_MUTEX_INITIALIZER;

int64_t fatsize;
dirent rootdir;
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

static int davfs_getattr(const char *path, struct stat *stbuf) {
    davfslogstr("Checking attributes on ", path);
    memset(stbuf,0,sizeof(struct stat));

    // lookup the directory entry
    dirent entry;
    int exit= traversepath(path, &rootdir, &entry);
    if (exit < 0)
        return exit;

    // populate the stat structure
    stbuf->st_mode = entry.type==DAV_DIR ? (S_IFDIR | PERM_MODE) : (S_IFREG | PERM_MODE);
    stbuf->st_nlink = 1;
    stbuf->st_size=entry.size;

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
    int ret= traversepath(dirn, &rootdir, &basedir);
    if (ret < 0)
        return ret;

    // get and zero new directory block
    ret= fatnewchain(&newdir.ptr);
    if (ret==0) {
        writeblock(&zeroblock, newdir.ptr);
        newdir.type = DAV_DIR;
        strcpy(newdir.name, basename(buff));
        free(buff);
    } else {
        free(buff);
        return ret;
    }

    davfslogstr("Dirname: ", newdir.name);
    return addInDirectory(&basedir,&newdir);
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

    char buffer[BLOCKSIZE];
    int blockcount=0;
    dirent entry;

    traversepath(path,&rootdir,&entry);
    blockptr blocktoread=entry.ptr;
    size_t realsize=(entry.size-offset) >= size ? size : entry.size-offset;
    size_t remaining=realsize;

    do {
        if (blockcount >= offset/BLOCKSIZE) {
            int blockoff = blockcount == offset/BLOCKSIZE ? offset % BLOCKSIZE : 0;
            readblock(&buffer,blocktoread);
            if (remaining > BLOCKSIZE - blockoff) {
                memcpy(buf + (realsize - remaining), buffer + blockoff, BLOCKSIZE - blockoff);
                remaining -= BLOCKSIZE;
            } else {
                memcpy(buf + (realsize - remaining), buffer + blockoff, remaining);
                return realsize;
            }
        }
        blocktoread= fatlookup(blocktoread);
        blockcount++;
    } while (blocktoread!=DAV_EOF);


    return realsize;
}

static int davfs_write(const char *path, const char *buff, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    davfslogstr("Writing ",path);
    int blockcount=0;
    size_t remaining=size;


    //update the directory record with the new size
    dirent file,containingdir;
    char *strbuff=strdup(path);
    char *base=basename(strbuff);
    char *dir=dirname(strbuff);
    traversepath(path,&rootdir,&file);

    int newbytes=offset+size-file.size;

    if (newbytes > 0) {
        file.size+=newbytes;
        traversepath(dir,&rootdir,&containingdir);
        updateDirectory(base,&rootdir,&file);
    }

    //computer the offsets
    int blockoffset=offset/BLOCKSIZE;
    blockptr startingblock;
    int ret=getblock(file.ptr, blockoffset, &startingblock, TRUE);
    if (ret < 0) {
        free(strbuff);
        return ret;
    }
    int headoffset=offset%BLOCKSIZE; //offset into the first block
    int tailoffset=BLOCKSIZE-((offset+size)%BLOCKSIZE);

    // read the first partial block
    char buffer[BLOCKSIZE];
    readblock(buffer,startingblock);
    if (size < BLOCKSIZE) {
        memcpy(buffer + headoffset, buff, BLOCKSIZE-headoffset-tailoffset);
        remaining-=BLOCKSIZE-headoffset-tailoffset;
    } else {
        memcpy(buffer + headoffset, buff, BLOCKSIZE-headoffset);
        remaining-=BLOCKSIZE-headoffset;
    }
    writeblock(buffer,startingblock);

    blockptr curblock=startingblock,previousblock;


    while (remaining > 0) {
        // extend the chain if required
        previousblock=curblock;
        curblock=fatlookup(previousblock);
        if (curblock==DAV_EOF) {
            ret= fatextendblocks(previousblock, &curblock);
            if (ret <0)
                return ret;
        }
        if (remaining < BLOCKSIZE) {
            readblock(buffer,curblock);
            memcpy(buffer,buff+size-remaining,remaining);
            writeblock(buffer,curblock);
            remaining=0;
        } else {
            writeblock(buff+size-remaining,BLOCKSIZE);
            remaining-=BLOCKSIZE;
        }
    }

    free(strbuff);
    return size;
}

static int davfs_open(const char *path, struct fuse_file_info *fi) {
    davfslogstr("Opening ", path);
    dirent file;

    int ret= traversepath(path, &rootdir, &file);
    if (ret<0)
        return ret;

    //fi->fh=gethandle();

    //filehandles[fi->fh]=file.ptr;
    return 0;
}

static int davfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
    davfslogstr("Reading directory: ", path);
    dirent dir;

    int ret= traversepath(path, &rootdir, &dir);
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
        readblock(&pair,searchblock);

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

static int davfs_release(const char *flags, struct fuse_file_info *fi) {
    //filehandles[fi->fh]=0;
    return 0;
}

static int davfs_statfs(const char *path, struct statvfs *stbuf) {
    davfslog("Statfs called");
    stbuf->f_bsize=BLOCKSIZE*8;
    stbuf->f_bavail=freeblocks;
    stbuf->f_blocks=super.numblocks;
    stbuf->f_files=10; // FIXME: lies
    stbuf->f_ffree=1000;
    stbuf->f_namemax=NAMELEN;
    return 0;

}

static int davfs_truncate(const char *path, off_t size) {
    dirent entry,dir;
    char *strbuf=strdup(path);
    char *base=basename(strbuf);
    char *dirstr=dirname(strbuf);

    int ret=traversepath(path,&rootdir,&entry);

    if (ret==0) {
        traversepath(dirstr,&rootdir,&dir);
        assert(size <= entry.size);
        entry.size = size;
        updateDirectory(base,&dir,&entry);
    }
    free(strbuf);
    return ret;
}

static int davfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char *pathbuff=strdup(path);

    char *base=basename(pathbuff);
    char *dir=dirname(pathbuff);

    dirent directory;

    int ret= traversepath(dir, &rootdir, &directory);

    if (ret==0) {
        // create file
        dirent newfile;

        newfile.size=0;
        ret=fatnewchain(&newfile.ptr);
        if (ret==0) {
            newfile.type = DAV_FILE;
            strcpy(newfile.name, base);

            ret = addInDirectory(&directory, &newfile);
        }
    }

    free(pathbuff);
    return ret;
}

static int davfs_unlink(const char *path) {
    davfslogstr("Removing ",path);
    dirent file,containingdir;
    int ret=traversepath(path,&rootdir,&file);
    if (ret < 0)
        return ret;

    // remove from directory
    char *buff=strdup(path);
    char *base=basename(buff);
    char *dirn=dirname(buff);
    traversepath(dirn,&rootdir,&containingdir);
    removeFromDirectory(base,&containingdir);

    // free blocks
    freechain(file.ptr);

    free(buff);
    return 0;
}

static int davfs_rmdir(const char *path) {
    davfslogstr("Removing ",path);
    dirent dir,containingdir;
    int ret=traversepath(path,&rootdir,&dir);
    if (ret < 0)
        return ret;

    // remove from directory
    char *buff=strdup(path);
    char *base=basename(buff);
    char *dirn=dirname(buff);
    traversepath(dirn,&rootdir,&containingdir);
    ret=removeFromDirectory(base,&containingdir);

    // free blocks
    freechain(dir.ptr);

    free(buff);
    return ret;
}

static struct fuse_operations davfsops = {
        .getattr	= davfs_getattr,
        .access		= davfs_access,
        .readlink	= davfs_readlink,
        .readdir	= davfs_readdir,
        .create     = davfs_create,
//        .mknod		= davfs_mknod,
        .mkdir		= davfs_mkdir,
//        .symlink	= davfs_symlink,
        .unlink		= davfs_unlink,
        .rmdir		= davfs_rmdir,
//        .chmod		= davfs_chmod,
//        .chown		= davfs_chown,
        .truncate	= davfs_truncate,
        .open		= davfs_open,
        .read		= davfs_read,
        .write		= davfs_write,
        .statfs		= davfs_statfs,
        .release	= davfs_release,
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
    fatinit();

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