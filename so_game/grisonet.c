#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "grisonet.h"

int griso_send(int desc, char* buf, int bytes_to_send){

    int written_bytes = 0; // index for reading from the buffer
    int bytes_left = bytes_to_send;

    while (bytes_left > 0) {
        int ret = send(desc, buf + written_bytes, bytes_left, 0);
        if(ret == -1){
            if(errno == EINTR)
                continue;
            return -1;
        }
        bytes_left -= ret;
        written_bytes += ret;
    }
    return written_bytes;
}


int griso_recv(int desc, char* buf, int bytes_to_recv){

    int read_bytes = 0; // writing index
    int bytes_left = bytes_to_recv;

    while (bytes_left > 0) {
        int ret = recv(desc, buf + read_bytes, bytes_left, 0);
        if(ret == -1){
            if(errno == EINTR)
                continue;
            return -1;
        }
        bytes_left -= ret;
        read_bytes += ret;
    }
    return read_bytes;
}
