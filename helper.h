//
// Created by David Tenty on 2016-04-06.
//

#ifndef DAVFS_HELPER_H
#define DAVFS_HELPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int davfs_initialize(int disk, uint32_t numblocks);

#ifdef __cplusplus
}
#endif
#endif //DAVFS_HELPER_H
