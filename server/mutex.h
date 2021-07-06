#ifndef MUTEX_H
#define MUTEX_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
  

void Pthread_mutex_lock(pthread_mutex_t *mtx){
    long err; 
    if ((err = pthread_mutex_lock(mtx) ) != 0 ) {
        errno = err;
        perror("lock"); 
        pthread_exit( (void*) err);
    }
}

void Pthread_mutex_unlock(pthread_mutex_t *mtx) {
    long err; 
    if ((err = pthread_mutex_unlock(mtx) ) != 0 ) {
        errno = err; 
        perror("unlock");
        pthread_exit( (void*) err );
    }
}

#endif