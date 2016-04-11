//
// Created by David Tenty on 2016-04-10.
//

#include "gtest/gtest.h"
#include "../globals.h"
#include "../fsoperations.h"
#include "../helper.h"
#include "../diskstructures.h"

class FSTest : public ::testing::Test {
protected:
    char inbuffer[BLOCKSIZE]="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc laoreet lobortis pharetra. Nam convallis feugiat tellus, vitae aliquet est placerat ac. Nam nec interdum risus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Aenean in mi tristique, pharetra lectus quis, pharetra mi. Duis ultrices, lacus ac mattis posuere, velit leo vulputate ante, sit amet egestas enim orci sodales risus. Fusce aliquam pharetra viverra. Duis gravida ex in metus suscipit, eu mattis lectus vulputate. Etiam eu amet";
    int disk;
    char *tempfile;
    const int blockcount=800;

    void initdisk() {
        disk=open(tempfile, O_WRONLY, S_IRWXU);
        ASSERT_NE(disk,-1) << strerror(errno);
        ASSERT_EQ(davfs_initialize(disk, blockcount),0);
        close(disk);
    }

    void mountdisk() {
        disk=open(tempfile, O_RDWR , S_IRWXU);
        ASSERT_NE(disk,-1) << strerror(errno);
        ASSERT_EQ(davfs_mount(disk),0) << strerror(errno);
    }

    FSTest() {
        char buffer[BLOCKSIZE];
        logfile=stdout;
        tempfile=strdup("davfs.XXXXXXX");
        disk=mkstemp(tempfile);
        for (int i=0; i < blockcount; i++) {
            write(disk,buffer,BLOCKSIZE);
        }
        close(disk);
    }

    void unmountdisk() {
        davfs_unmount();
        close(disk);
    }

    blockptr *dupfat() {
        blockptr *clone=(blockptr*)malloc(fatsize*BLOCKSIZE);
        memcpy(clone,fat,fatsize*BLOCKSIZE);
        return clone;
    }

    void validatefat(blockptr *expectedfat) {
        for (int i=0; i < super.numblocks; i++) {
            SCOPED_TRACE(i);
            EXPECT_EQ(expectedfat[i],fat[i]);
        }
    }

    ~FSTest() {
        free(tempfile);
    }
};

TEST_F(FSTest,DirectoryCreation) {
    ASSERT_NO_FATAL_FAILURE(initdisk());
    ASSERT_NO_FATAL_FAILURE(mountdisk());
    ASSERT_EQ(davfs_mkdir("/foo",777),0) << strerror(errno);
    unmountdisk();
}

TEST_F(FSTest,ReadWriteFile) {
    fuse_file_info file;

    char outbuffer[BLOCKSIZE];

    ASSERT_NO_FATAL_FAILURE(initdisk());
    ASSERT_NO_FATAL_FAILURE(mountdisk());
    ASSERT_EQ(davfs_create("/foo.txt",777,&file),0) << strerror(errno);
    ASSERT_EQ(davfs_write("/foo.txt",inbuffer,BLOCKSIZE,0,&file),BLOCKSIZE);
    ASSERT_EQ(davfs_read("/foo.txt",outbuffer,BLOCKSIZE,0,&file),BLOCKSIZE);
    ASSERT_EQ(davfs_release("/foo.txt",&file),0);
    ASSERT_STREQ(inbuffer,outbuffer);
    unmountdisk();
}

TEST_F(FSTest,AddRemoveDirectory) {
    ASSERT_NO_FATAL_FAILURE(initdisk());
    ASSERT_NO_FATAL_FAILURE(mountdisk());
    blockptr *beforefat=dupfat();
    ASSERT_EQ(davfs_mkdir("/foo",777),0) << strerror(errno);
    ASSERT_EQ(davfs_rmdir("/foo"),0) << strerror(errno);
    validatefat(beforefat);
    free(beforefat);
    unmountdisk();
}

TEST_F(FSTest,AddRemoveFile) {
    fuse_file_info file;
    ASSERT_NO_FATAL_FAILURE(initdisk());
    ASSERT_NO_FATAL_FAILURE(mountdisk());
    blockptr *beforefat=dupfat();
    ASSERT_EQ(davfs_create("/foo.txt",777,&file),0) << strerror(errno);
    ASSERT_EQ(davfs_release("/foo.txt",&file),0) << strerror(errno);
    ASSERT_EQ(davfs_unlink("/foo.txt"),0) << strerror(errno);
    validatefat(beforefat);
    free(beforefat);
    unmountdisk();
}

TEST_F(FSTest,AddRemoveFileWithData) {
    fuse_file_info file;
    ASSERT_NO_FATAL_FAILURE(initdisk());
    ASSERT_NO_FATAL_FAILURE(mountdisk());
    blockptr *beforefat=dupfat();
    ASSERT_EQ(davfs_create("/foo.txt",777,&file),0) << strerror(errno);
    ASSERT_EQ(davfs_write("/foo.txt",inbuffer,BLOCKSIZE,0,&file),BLOCKSIZE);
    ASSERT_EQ(davfs_write("/foo.txt",inbuffer,BLOCKSIZE,512,&file),BLOCKSIZE);
    ASSERT_EQ(davfs_release("/foo.txt",&file),0) << strerror(errno);
    ASSERT_EQ(davfs_unlink("/foo.txt"),0) << strerror(errno);
    validatefat(beforefat);
    free(beforefat);
    unmountdisk();
}