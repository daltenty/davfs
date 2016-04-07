//
// Created by David Tenty on 2016-04-07.
//

#include "logging.h"

void davfslog(const char *string) {
    pthread_mutex_lock(&logmutex);
    fprintf(logfile,"%s\n",string);
    fflush(logfile);
    pthread_mutex_unlock(&logmutex);
}

void davfslogstr(const char *string1,const char *string2) {
    pthread_mutex_lock(&logmutex);
    fprintf(logfile,"%s%s\n",string1,string2);
    fflush(logfile);
    pthread_mutex_unlock(&logmutex);
}

void davfslognum(const char *string1,uint64_t num) {
    pthread_mutex_lock(&logmutex);
    fprintf(logfile,"%s%d\n",string1,num);
    fflush(logfile);
    pthread_mutex_unlock(&logmutex);
}