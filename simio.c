//
// Created by David Tenty on 2016-04-11.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "diskstructures.h"

#define NUMFILES 5


int main(int argc,char *argv[]) {
    int numops;
    char *buff;
    char inbuffer[BLOCKSIZE]="Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc laoreet lobortis pharetra. Nam convallis feugiat tellus, vitae aliquet est placerat ac. Nam nec interdum risus. Interdum et malesuada fames ac ante ipsum primis in faucibus. Aenean in mi tristique, pharetra lectus quis, pharetra mi. Duis ultrices, lacus ac mattis posuere, velit leo vulputate ante, sit amet egestas enim orci sodales risus. Fusce aliquam pharetra viverra. Duis gravida ex in metus suscipit, eu mattis lectus vulputate. Etiam eu amet";
    char outbuffer[BLOCKSIZE];

    int files[NUMFILES];

    srandomdev();

    if (argc != 2) {
        printf("Usage: %s [num io ops]\n",argv[0]);
    }
    numops=atoi(argv[1]);

    // create the files
    for (int i=0; i < NUMFILES; i++) {
        buff=strdup("rand.XXXXX");
        files[i]=mkstemp(buff);
        free(buff);
    }

    // preform the i/o
    for (int j=0; j < numops; j++ ) {
        int choice = random();
        int fileid = random() % NUMFILES;
        int size = random() % 512;

        if (choice % 2 == 0) {
            write(files[fileid],inbuffer,size);
        } else {
            read(files[fileid],inbuffer,size);
        }
    }

    return 0;
}