
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"

/*
 * global variables
 */

int window;
World world;
Vehicle* vehicle; // The vehicle

int socket_desc;

//todo can convert to bool? (stdbool.h)
int connectivity = 0;

//--------------------------------------->

/*
 * TCP functions
 */

int getIdFromServer(){
  return 0;
}

Image* getMapElevationFromServer(){
  return NULL;
}

Image* getMapTextureFromServer(){
  return NULL;
}

Image* getMyTextureFromServer(Image* my_texture_for_server, int id){
  return NULL;
}

//--------------------------------------->

/*
 * UDP threads routines
 */

void* UDPSender(void* args){
  return NULL;
}

void* UDPReceiver(void* args){
  return NULL;
}

//--------------------------------------->

/*
 * Signal Handling
 */
 void signalHandler(int signal){
   switch (signal) {
     case SIGHUP:
       break;
     case SIGINT:
       connectivity=0;
       WorldViewer_exit(0);
       break;
     default:
        fprintf(stderr, "Uncaught signal: %d\n", signal);
        return;
     }
 }

//--------------------------------------->

int main(int argc, char **argv) {
  if (argc<3) {
    printf("usage: %s <server_address> <player texture>\n", argv[1]);
    exit(-1);
  }

  printf("loading texture image from %s ... ", argv[2]);
  Image* my_texture = Image_load(argv[2]);
  if (my_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }

  //fixme is this needed?
  Image* my_texture_for_server= my_texture;

  // todo: connect to the server
  //   -get ad id
  //   -send your texture to the server (so that all can see you)
  //   -get an elevation map
  //   -get the texture of the surface

  // these come from the server
  int my_id = getIdFromServer();
  Image* map_elevation= getMapElevationFromServer();
  Image* map_texture= getMapTextureFromServer();
  Image* my_texture_from_server= getMyTextureFromServer(my_texture_for_server, my_id);

  // construct the world
  World_init(&world, map_elevation, map_texture, 0.5, 0.5, 0.5);
  vehicle=(Vehicle*) malloc(sizeof(Vehicle));
  Vehicle_init(vehicle, &world, my_id, my_texture_from_server);
  World_addVehicle(&world, vehicle);

  // spawn a thread that will listen the update messages from
  // the server, and sends back the controls
  // the update for yourself are written in the desired_*_force
  // fields of the vehicle variable
  // when the server notifies a new player has joined the game
  // request the texture and add the player to the pool
  /*FILLME*/

  WorldViewer_runGlobal(&world, vehicle, &argc, argv);

  // cleanup
  World_destroy(&world);
  return 0;
}
