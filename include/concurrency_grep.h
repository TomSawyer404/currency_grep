#ifndef _CONCURRENCY_GREP_H
#define _CONCURRENCY_GREP_H

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>  // get_nprocs()
#include <unistd.h>

typedef struct config_st {
    char* file_pointer;
    char* pattern;
    u_int32_t cpu_num;
    u_int32_t file_size;
    u_int32_t unit;  // how many bytes are there in a basic chunk unit
    u_int32_t seq;
} Config;

int grep_init(char** argv);
#endif
