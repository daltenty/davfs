//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_DIRECTORYOPERATIONS_H
#define DAVFS_DIRECTORYOPERATIONS_H

#include "globals.h"

int findInDirectory(const char *name, const dirent *dir, dirent *result);
int traversepath(const char *path, const dirent *base, dirent *dir);
int addInDirectory(const dirent *directory, const dirent *entry);
int updateDirectory(const char *name, const dirent *dir, const dirent *entry);
int removeFromDirectory(const char *name, const dirent *dir);
#endif //DAVFS_DIRECTORYOPERATIONS_H
