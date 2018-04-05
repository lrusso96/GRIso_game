#pragma once

#include <netinet/in.h>

#include "image.h"
#include "vehicle.h"
#include "world.h"



typedef struct ClientItem {
    struct sockaddr_in user_addr;
    int id;
} ClientItem;

typedef struct WorldServer {
    ListHead clients;
    World* w;

} WorldServer;




WorldServer* WorldServer_init(Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step);

int WorldServer_addClient(WorldServer* ws, Vehicle* v, struct sockaddr_in user_addr);
int WorldServer_detachClient(WorldServer* ws, int id);

void WorldServer_destroy(WorldServer* ws);

//debug
void WorldServer_print(WorldServer* ws);
