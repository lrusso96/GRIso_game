#pragma once

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
    World* w;

    //cached stuff
    int online_users;
} WorldExtended;

WorldExtended* WorldExtended_init(Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step);

void WorldExtended_destroy(WorldExtended* we);

int WorldExtended_addVehicle(WorldExtended* we, Vehicle* v);
int WorldExtended_detachVehicle(WorldExtended* we, int id);


//debug
void WorldExtended_print(WorldExtended* we);


//getForces
void WorldExtended_getVehicleForcesUpdate(WorldExtended* we, Vehicle* v, float* tf, float* rf);

//todo setForces (it's not needed if the work is done server side)

//getXYT (not needed if the work is done server side)
void WorldExtended_getVehicleXYT(WorldExtended* we, Vehicle* v, float* x, float* y, float* t);

//todo setXYT


void WorldExtended_vehicleUpdatePacket_init(WorldExtended* we, VehicleUpdatePacket*, Vehicle* v);











