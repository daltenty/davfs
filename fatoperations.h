//
// Created by David Tenty on 2016-04-07.
//

#ifndef DAVFS_FATOPERATIONS_H
#define DAVFS_FATOPERATIONS_H

#include "globals.h"

blockptr fatlookup(const blockptr entry);
blockptr fatextendblocks(const blockptr entry);
blockptr fatnewchain();
blockptr getblock(blockptr start, int n);

#endif //DAVFS_FATOPERATIONS_H
