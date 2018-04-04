#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "local_world.h"
#include "vehicle.h"
#include "world.h"


LocalWorld* LocalWorld_init(void){
    LocalWorld* lw = (LocalWorld*)malloc(sizeof(LocalWorld));
    lw->vehicles=(Vehicle**)malloc(sizeof(Vehicle*)*LOCALWORLD_SIZE);
    lw->ids = (int*) malloc(sizeof(int)*LOCALWORLD_SIZE);
    lw->available_ids = (bool*) malloc(sizeof(bool)*LOCALWORLD_SIZE);
    for(int i=0;i<LOCALWORLD_SIZE;i++){
        lw->ids[i]=-1;
        lw->available_ids[i] = true;
    }
    lw->size = LOCALWORLD_SIZE;
    return lw;
}

void LocalWorld_resize(LocalWorld* lw){
    int s = lw->size * 2;
    lw->size = s;
    lw->vehicles = realloc(lw->vehicles, sizeof(Vehicle*)*s);
    lw->ids = realloc(lw->ids, sizeof(int)*s);
    lw->available_ids = realloc(lw->ids, sizeof(bool)*s);
    for(int i=s; i<s; i++)
        lw->available_ids[i] = true;

}

void LocalWorld_destroy(LocalWorld* lw, World* w){
    int size = lw->size;
    for(int i=0;i<size;i++){
        if(lw->available_ids[i])
            continue;

        Image* im = lw->vehicles[i]->texture;
        World_detachVehicle(w,lw->vehicles[i]);
        if(im)
            Image_free(im);
        Vehicle_destroy(lw->vehicles[i]);
        free(lw->vehicles[i]);
    }
    free(lw->ids);
    free(lw->available_ids);
    free(lw->vehicles);
    free(lw);
}

int getFirstAvailableId(LocalWorld* lw){
    int size = lw->size;
    for(int i=0; i<size; i++){
        if(lw->available_ids[i])
            return i;
    }
    return -1;
}


int LocalWorld_addVehicle(LocalWorld* lw, Vehicle* v){
    int id = v->id;
    printf("%d id\n", id);
    int pos = getFirstAvailableId(lw);
    if(pos<0){
        pos = lw->size;
        LocalWorld_resize(lw);
    }
    lw->ids[pos] = id;
    lw->available_ids[pos] = false;
    lw->vehicles[pos] = v;
    lw->online_users++;
    return pos;
}

void LocalWorld_detachVehicle(LocalWorld* lw, World* w, int id){
    int size = lw->size;
    for(int i=0;i<size;i++){
        if(!lw->available_ids[i] && lw->ids[i] == id){
            Image* im = lw->vehicles[i]->texture;
            World_detachVehicle(w,lw->vehicles[i]);
            if(im)
                Image_free(im);
            Vehicle_destroy(lw->vehicles[i]);
            free(lw->vehicles[i]);

            lw->available_ids[i] = true;
            lw->online_users--;
            break;
        }
    }
}

void LocalWorld_print(LocalWorld* lw){
    printf("--- Local world of size %d---\n\n", lw->size);
    int left = lw->online_users;
    printf("%d online users\n", left);

    int i = 0;
    while(left){
        if(!lw->available_ids[i]){
            printf("\n\tuser %d", lw->ids[i]);
            left--;
        }
        i++;
    }

    printf("\n\n");


}











