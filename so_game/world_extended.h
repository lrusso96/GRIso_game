#pragma once

#include <stdbool.h>

#include "so_game_protocol.h"
#include "world.h"

/*
 * This module is a wrapper of @World
 * It can improve the performance, caching some important values
 *
 * This was made to not modify World.h module (provided by Grisetti)
 *
 */
typedef struct WorldExtended {
    World* w;   //local world
    int online_users;   //number of users
    int max_id;         //max id value in world (cached to handle segmentation fault)
    bool* ids;          //if ids[k] == true, then k is a valid id in the world. Need more?
    bool* with_texture; //same as ids, but this deals with texture vehicles (of "k-th" id)
} WorldExtended;


//----------CORE functions--------------------------------------------->

//create a new object WorldExtended (unique for game session actually)
WorldExtended* WorldExtended_init(Image* surface_elevation, Image* surface_texture, float x_step, float y_step, float z_step);

//destroy the object (and all stuff inside it). Valgrind-tested!
void WorldExtended_destroy(WorldExtended* we);

//add or detahc vehicles to world object
int WorldExtended_addVehicle(WorldExtended* we, Vehicle* v);
int WorldExtended_detachVehicle(WorldExtended* we, int id);

//get updated state of vehicle (my own, actually) and fill a "wup" object
void WorldExtended_vehicleUpdatePacket_init(WorldExtended* we, VehicleUpdatePacket*, Vehicle* v);

//work with bools insde world
int WorldExtended_HasIdAndTexture(WorldExtended* we, int id);



//----------some debug------------------------------------------------->

//prints the state of the world
void WorldExtended_print(WorldExtended* we);

//getForces
void WorldExtended_getVehicleForcesUpdate(WorldExtended* we, Vehicle* v, float* tf, float* rf);

//todo setForces

//getXYT
void WorldExtended_getVehicleXYTPlus(WorldExtended* we, Vehicle* v, float* x, float* y, float* t, float* rf, float* tf);

//todo setXYT
void WorldExtended_setVehicleXYTPlus(WorldExtended* we, int id, float x, float y, float t, float rf, float tf);
