
#include <string.h>
#include <errno.h>

#define SERVER_ADDRESS  "127.0.0.1"
#define SERVER_TCP_PORT 2018
#define SERVER_UDP_PORT 3001
#define BUFFER_SIZE 500000
#define MAX_CONNECTIONS 10


#define GENERIC_ERROR_HELPER(cond, errCode, msg) do {               \
        if (cond) {                                                 \
            fprintf(stderr, "%s: %s\n", msg, strerror(errCode));    \
            exit(EXIT_FAILURE);                                     \
        }                                                           \
    } while(0)

#define ERROR_HELPER(ret, msg)      GENERIC_ERROR_HELPER((ret < 0), errno, msg)

#define PTHREAD_ERROR_HELPER(ret, msg)  GENERIC_ERROR_HELPER((ret != 0), ret, msg)
