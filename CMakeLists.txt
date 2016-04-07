cmake_minimum_required(VERSION 3.4)
project(davfs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(FUSE REQUIRED)
include_directories(${FUSE_INCLUDE_DIRS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUSE_DEFINITIONS} -std=c99")

set(SOURCE_FILES mountdavfs.c helper.c helper.h)
link_directories(/usr/local/lib/)
add_executable(davfs ${SOURCE_FILES})
add_executable(mkdavfs mkdavfs.c helper.c)
target_link_libraries(davfs ${FUSE_LIBRARIES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(davfs osxfuse)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")