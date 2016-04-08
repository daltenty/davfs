//
// Created by David Tenty on 2016-04-07.
//

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "diskstructures.h"
#include "directoryoperations.h"
#include "logging.h"
#include "blockoperations.h"
#include "fatoperations.h"

/*
 * Given a directory and name, update the entry in the directory
 *
 * /param name the name to search for
 * /param dir the directory to search
 * /param entry the dirent to update
 * /return zero if successful, -errno elsewise
 */
int updateDirectory(const char *name, const dirent *dir, const dirent *entry) {
    assert(strcmp(name, "") != 0); // filenames cannot be blank
    davfslogstr("Updating entry ", name);
    dirpair pair;


    blockptr searchblock=dir->ptr;
    do {
        //load the directory pair
        readblock(&pair, searchblock);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type != DAV_INVALID && strcmp(pair.entries[i].name, name) == 0) {
                memcpy(pair.entries + i, entry,sizeof(dirent));
                writeblock(&pair,searchblock);
                return 0;
            }
        }

        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    return -ENOENT;
}

/*
 * Given a directory and name, search for the entry in the directory
 *
 * /param name the name to search for
 * /param dir the directory to search
 * /param result the dirent to fill with results
 * /return zero if successful, -errno elsewise
 */
int findInDirectory(const char *name, const dirent *dir, dirent *result) {
    assert(strcmp(name, "") != 0); // filenames cannot be blank
    davfslogstr("Searching for entry ", name);
    dirpair pair;


    blockptr searchblock=dir->ptr;
    do {
        //load the directory pair
        readblock(&pair, searchblock);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type != DAV_INVALID && strcmp(pair.entries[i].name, name) == 0) {
                memcpy(result, pair.entries + i, sizeof(dirent));
                return 0;
            }
        }

        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    return -ENOENT;
}


/*
 * Given a directory and an entry, record the entry
 *
 * /param directory the directory to record the entry in
 * /param entry the entry to record
 * /return zero if successful, -errno elsewise
 */
int addInDirectory(const dirent *directory, const dirent *entry) {
    dirpair pair;

    blockptr prevblock;
    blockptr searchblock=directory->ptr;
    do {
        //load the directory pair
        readblock(&pair, searchblock);

        for (int i = 0; i < 2; i++) {
            if (pair.entries[i].type == DAV_INVALID ) {
                memcpy(pair.entries + i, entry, sizeof(dirent));
                writeblock(&pair,searchblock);
                return 0;
            }
        }
        prevblock=searchblock;
        searchblock= fatlookup(searchblock);
    } while (searchblock!=DAV_EOF);

    // got to EOF, need to extend directory chain
    searchblock=fatextendblocks(prevblock);
    memset(&pair,0, sizeof(dirpair));
    memcpy(pair.entries,entry, sizeof(dirent));
    writeblock(&pair,searchblock);

    return 0;
}

/*
 * Given a partial path search for the directory entry
 * relative to base
 *
 * /param path the partial path to search for
 * /param base the dir to search
 * /param dir the dirent to fill with results
 * /return zero if successful, -errno elsewise
 */
int traversepath(const char *path, const dirent *base, dirent *dir) {
    //davfslogstr("Traversing path ", path);
    dirent basedir;

    // check if we are searching for the root dir
    if (strcmp(path, "/") == 0) {
        assert(base==&rootdir);
        memcpy(dir, base, sizeof(dirent));
        return 0;
    }

    // slice the path into head and tail
    char *headbuff= strdup(path);

    char *head=headbuff+1;
    char *tail=NULL;

    char *sep= index(head, '/');
    if (sep!=NULL) {
        *sep = '\0';
         tail= index(path + 1, '/');
    }

    int ret = findInDirectory(head, base, &basedir);
    free(headbuff);
    if (ret < 0) {
        return ret; // not found
    }
    else if (tail!=NULL) {
        return traversepath(tail, &basedir, dir); // recurse on tail
    }
    else {
        memcpy(dir,&basedir, sizeof(dirent)); // found
        return 0;
    }
}