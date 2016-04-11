//
// Created by David Tenty on 2016-04-11.
//

#ifndef DAVFS_FSOPERATIONS_H
#define DAVFS_FSOPERATIONS_H

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

extern struct fuse_operations davfsops;

#ifdef __cplusplus
extern "C" {
#endif

int davfs_mount(int device);
void davfs_unmount();
int davfs_getattr(const char *path, struct stat *stbuf);
int davfs_rmdir(const char *path);
int davfs_unlink(const char *path);
int davfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int davfs_truncate(const char *path, off_t size);
int davfs_statfs(const char *path, struct statvfs *stbuf);
int davfs_release(const char *flags, struct fuse_file_info *fi);
int davfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int davfs_open(const char *path, struct fuse_file_info *fi);
int davfs_write(const char *path, const char *buff, size_t size, off_t offset,
                struct fuse_file_info *fi);
int davfs_read(const char *path, char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);
int davfs_chmod(const char *path, mode_t mode);
int davfs_chown(const char *path, uid_t uid, gid_t gid);
int davfs_mknod(const char *path, mode_t mode, dev_t rdev);
int davfs_symlink(const char *from, const char *to);
int davfs_readlink(const char *path, char *buf, size_t size);
int davfs_access(const char *path, int mask);
int davfs_mkdir(const char *path, mode_t mode);


#endif //DAVFS_FSOPERATIONS_H
#ifdef __cplusplus
}
#endif