CPS 801 Final Project
David Tenty 500540547

This project implements a simple FAT based filesystem against the Filesystem in User Space (FUSE) interfaces. Compiling
this package requires the FUSE (or OSXFuse) libraries as well as the CMake build system.

(Test builds were preformed with CLion)

Part 1: This project implements the filesystems and stores the data in a file or blockdevice. Standard unix commands
(ls, mkdir, etc.) can be used to perform operations.

Example usage:

$ dd if=/dev/zero of=davfs.disk bs=512 count=800  # create the disk file
$ ./mkdavfs ./davfs.disk  # format the disk, creating the internal datastructures
$ ./davfs ./davfs.disk /Volumes/davfs -f # mount and run in the foreground so we can see output



