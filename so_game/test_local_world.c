#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include "local_world.h"
#include "vehicle.h"
#include "world.h"


int main(int argc, char const * argv[]){

    World* w = (World*) malloc(sizeof(World));
    Image* i1 = (Image*) malloc(sizeof(Image));
    Image* i2 = (Image*) malloc(sizeof(Image));
    World_init(w, i1, i2, 10, 10, 10);

    LocalWorld* lw = LocalWorld_init();
    LocalWorld_print(lw);


    Image* i3 = (Image*) malloc(sizeof(Image));
    Vehicle* v1 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v1, w, 101, i3);

    Image* i4 = (Image*) malloc(sizeof(Image));
    Vehicle* v2 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v2, w, 46, i4);

    LocalWorld_addVehicle(lw, v1);
    LocalWorld_print(lw);

    LocalWorld_addVehicle(lw, v2);
    LocalWorld_print(lw);

    LocalWorld_detachVehicle(lw, w, 101);
    LocalWorld_print(lw);

    LocalWorld_destroy(lw, w);
    World_destroy(w);

    free(i1);
    free(i2);
    free(w);

}
