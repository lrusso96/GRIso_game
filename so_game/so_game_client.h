#pragma once

typedef struct localWorld{
  int  ids[256];
  Vehicle** vehicles;
  int online_users;
}localWorld;

typedef struct udpArgs{
  struct sockaddr_in server_addr;
  localWorld* lw;
  int socket_udp;
  int socket_tcp;
}udpArgs;
