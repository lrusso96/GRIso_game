#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "world.h"
#include "world_extended.h"


WorldExtended* WorldExtended_init(Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step){

    WorldExtended* we = (WorldExtended*) malloc(sizeof(WorldExtended));
    we->w = (World*) malloc(sizeof(World));
    World_init(we->w, surface_elevation, surface_texture, x_step, y_step, z_step);
    we->online_users = 0;
    return we;

}
void WorldExtended_destroy(WorldExtended* we){

    World* w = we->w;

    ListItem* item=w->vehicles.first;
    while(item){
        Vehicle* v=(Vehicle*)item;
        item = item->next;
        WorldExtended_detachVehicle(we, v->id);
        printf("pippo\n");
  }


    //when world is empty
    World_destroy(we->w);
    free(we->w);
    free(we);
}

int WorldExtended_addVehicle(WorldExtended* we, Vehicle* v){
    World_addVehicle(we->w, v);
    int s = we->w->vehicles.size;
    we->online_users = s;
    return s;
}

int WorldExtended_detachVehicle(WorldExtended* we, int id){
    Vehicle* v = World_detachVehicle(we->w, World_getVehicle(we->w, id));
    if(v){
        Image_free(v->texture);
        free(v);
    }
    int s = we->online_users = we->w->vehicles.size;
    we->online_users = s;
    return s;
}

void WorldExtended_print(WorldExtended* we){
    World* w = we->w;

    printf("---- World with %d online users ----\n", we->online_users);
    ListItem* item=w->vehicles.first;
    while(item){
        Vehicle* v=(Vehicle*)item;
        printf("\tvehicle %d\n", v->id);
        item=item->next;
  }
}
