#pragma once
#include <netinet/in.h>

#include "image.h"
#include "vehicle.h"

//need some more?
typedef struct ClientListItem {
    struct ClientListItem* prev;
    struct ClientListItem* next;
    Vehicle* vehicle;
    Image * v_texture;
    struct sockaddr_in user_addr;
    int id;
} ClientListItem;

typedef struct ClientListHead {
  ClientListItem* first;
  ClientListItem* last;
  int size;
} ClientListHead;

void ClientList_init(ClientListHead* head);
ClientListItem* ClientList_find(ClientListHead* head, ClientListItem* item);
ClientListItem* ClientList_append(ClientListHead* head, ClientListItem* item);
ClientListItem* ClientList_detach(ClientListHead* head, ClientListItem* item);
void ClientList_destroy(ClientListHead* head);
void ClientList_print(ClientListHead* head);
