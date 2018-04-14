#pragma once

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SERVER_ADDRESS  "127.0.0.1"     //localhost: it's where server runs
#define SERVER_TCP_PORT 2018            //number of port where server listens (tcp)
#define SERVER_UDP_PORT 3001            //number of port where server listens (udp)
#define BUFFER_SIZE 500000              //buffer length, used both sender and receiver side
#define MAX_CONNECTIONS 10

// easy way to handle generic errors
#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

// wrapper of generic error helper. Used in most calls
#define ERROR_HELPER(ret, msg)      GENERIC_ERROR_HELPER((ret < 0), errno, msg)

// As Error Helper, but this deals with pthread errors (ret can be > 0 too!)
#define PTHREAD_ERROR_HELPER(ret, msg)  GENERIC_ERROR_HELPER((ret != 0), ret, msg)
