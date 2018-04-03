#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include "vehicle.h"


//is this enough?
typedef struct localWorld{
  int  ids[256];
  Vehicle** vehicles;
  int online_users;
}localWorld;

//need some more?
typedef struct udpArgs{
  struct sockaddr_in server_addr;
  localWorld* lw;
  int socket_udp;
  int socket_tcp;
}udpArgs;
