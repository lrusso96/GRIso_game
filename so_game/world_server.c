#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "world.h"
#include "world_server.h"

WorldServer* WorldServer_init(Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step){

    WorldServer* ws = (WorldServer*) malloc(sizeof(WorldServer));
    ws->w = (World*) malloc(sizeof(World));
    World_init(ws->w, surface_elevation, surface_texture, x_step, y_step, z_step);

    List_init(&ws->clients);

    return ws;

}


int WorldServer_addClient(WorldServer* ws, Vehicle* v, struct sockaddr_in user_addr){
    World_addVehicle(ws->w, v);
    ClientItem* ci = (ClientItem*) malloc(sizeof(ClientItem));
    int id = v->id;
    ci->id = id;
    ci->user_addr = user_addr;
    List_append(&ws->clients, (ListItem*) ci);
    return id;

}

int WorldServer_detachClient(WorldServer* ws, int id){
    Vehicle* v = World_detachVehicle(ws->w, World_getVehicle(ws->w, id));

    if(v){
        Image_free(v->texture);
        free(v);
    }
    ListItem* item = ws->clients.first;

    while(item){
        ClientItem* ci=(ClientItem*)item;

        item = item->next;

        if(ci->id==id){
            List_detach(&(ws->clients), (ListItem*) ci);
            free(ci);
            return id;
        }

    }
    return id;
}


void WorldServer_destroy(WorldServer* ws){
    World* w = ws->w;

    ListItem* item=w->vehicles.first;
    while(item){
        Vehicle* v=(Vehicle*)item;
        item = item->next;
        WorldServer_detachClient(ws, v->id);
  }


    //when world is empty
    World_destroy(ws->w);
    free(ws->w);

    List_destroy(&ws->clients);

    //free(ws);
}

void WorldServer_print(WorldServer* ws){
    World* w = ws->w;

    printf("---- World with %d online users ----\n", ws->clients.size);
    ListItem* item=w->vehicles.first;
    while(item){
        Vehicle* v=(Vehicle*)item;
        printf("\tvehicle %d\n", v->id);
        item=item->next;
    }
    printf("\n");
}


