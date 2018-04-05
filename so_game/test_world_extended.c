#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include "world_extended.h"
#include "vehicle.h"
#include "world.h"


int main(int argc, char const * argv[]){

    Image* i1 = (Image*) malloc(sizeof(Image));
    Image* i2 = (Image*) malloc(sizeof(Image));

    WorldExtended* we = WorldExtended_init(i1, i2, 0.1, 0.2, 0.3);

    Image* i3 = (Image*) malloc(sizeof(Image));
    Vehicle* v1 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v1, we->w, 101, i3);

    Image* i4 = (Image*) malloc(sizeof(Image));
    Vehicle* v2 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v2, we->w, 46, i4);

    WorldExtended_addVehicle(we, v1);
    WorldExtended_print(we);

    WorldExtended_addVehicle(we, v2);
    WorldExtended_print(we);

    WorldExtended_detachVehicle(we, 101);
    WorldExtended_print(we);

    WorldExtended_destroy(we);

    free(i1);
    free(i2);

}
