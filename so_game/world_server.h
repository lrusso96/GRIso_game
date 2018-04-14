#pragma once

#include <netinet/in.h>
#include <pthread.h>

#include "image.h"
#include "vehicle.h"
#include "world.h"


/*
 * This module is a wrapper of @World
 * It manages a @linked_list of network infos of the clients.
 *
 * This was made to not modify World.h module (provided by Grisetti)
 *
 */
typedef struct ClientItem {
    struct sockaddr_in user_addr;           //client address
    int id;                                 //global id of client
    int socket;                             //tcp socket
    struct sockaddr_storage clientStorage;  //udp socket
    socklen_t addr_size;                    //udp socklen (cahced to not call sizeof all times)

    //pthread_t* tcp_thread; to handle server quit (later on)
} ClientItem;

typedef struct WorldServer {
    ListHead clients;           //list of ClientItem objects
    World* w;                   //the world (never displayed, but often CRUD'ed)
    pthread_mutex_t mutex;      //who locks this can do critical stuff without race condition
} WorldServer;

//----------CORE functions--------------------------------------------->

//create a new object WorldServer (globally unique)
WorldServer* WorldServer_init(Image* surface_elevation, Image* surface_texture, float x_step, float y_step, float z_step);

//add / detach ClientItem object
int WorldServer_addClient(WorldServer* ws, Vehicle* v, ClientItem* ci);
int WorldServer_detachClient(WorldServer* ws, int id);
//apply updates to vehicle in the world
int WorldServer_updateClient(WorldServer* ws, int id, float x, float y, float t);
//retrieve info of some vehcile (used to create a "epoch state"
int WorldServer_getClientInfo(WorldServer* ws, int id, float* x, float* y, float* t);
//destroy world. Valgrind-tested
void WorldServer_destroy(WorldServer* ws);

//print the state of world (debug only)
void WorldServer_print(WorldServer* ws);
