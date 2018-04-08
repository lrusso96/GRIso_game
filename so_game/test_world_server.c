#include <netinet/in.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>


#include "vehicle.h"
#include "world.h"
#include "world_server.h"


int main(int argc, char const * argv[]){

    Image* i1 = (Image*) malloc(sizeof(Image));
    Image* i2 = (Image*) malloc(sizeof(Image));

    WorldServer* ws = WorldServer_init(i1, i2, 0.1, 0.2, 0.3);

    Image* i3 = (Image*) malloc(sizeof(Image));
    Vehicle* v1 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v1, ws->w, 101, i3);

    Image* i4 = (Image*) malloc(sizeof(Image));
    Vehicle* v2 = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(v2, ws->w, 46, i4);

    struct sockaddr_in* u1 = {0};
    struct sockaddr_in* u2 = {0};


    WorldServer_addClient(ws, v1, u1);
    WorldServer_print(ws);

    WorldServer_addClient(ws, v2, u2);
    WorldServer_print(ws);

    WorldServer_detachClient(ws, 101);
    WorldServer_print(ws);

    WorldServer_detachClient(ws, 46);
    WorldServer_print(ws);

    WorldServer_destroy(ws);

    free(i1);
    free(i2);

}
