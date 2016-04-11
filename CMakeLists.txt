cmake_minimum_required(VERSION 3.4)
project(davfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_subdirectory(vendor/googletest/googletest)

find_package(FUSE REQUIRED)

include_directories(${FUSE_INCLUDE_DIRS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUSE_DEFINITIONS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FUSE_DEFINITIONS} -std=c++11")

set(DAVFS_SOURCE_FILES helper.c logging.c fatoperations.c
        blockoperations.c directoryoperations.c fsoperations.c)
set(MKDAVFS_SOURCE_FILES helper.c)

link_directories(/usr/local/lib/)
add_executable(davfs mountdavfs.c ${DAVFS_SOURCE_FILES})
add_executable(mkdavfs mkdavfs.c  ${MKDAVFS_SOURCE_FILES})
add_executable(simio simio.c)
add_executable(davtest test/davtest.cpp ${MKDAVFS_SOURCE_FILES} ${DAVFS_SOURCE_FILES})
target_link_libraries(davfs ${FUSE_LIBRARIES})
target_link_libraries(davtest gtest gtest_main )

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(davfs osxfuse)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
