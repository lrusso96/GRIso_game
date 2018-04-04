#pragma once

#include <stdbool.h>

#include "vehicle.h"
#include "world.h"

#define LOCALWORLD_SIZE 256

typedef struct LocalWorld{
  int*  ids;            //ids[0] is my own id
  bool* available_ids;
  Vehicle** vehicles;
  int online_users;
  int size;
}LocalWorld;

LocalWorld* LocalWorld_init(void);
void LocalWorld_destroy(LocalWorld* lw, World* w);
void LocalWorld_resize(LocalWorld* lw);

int LocalWorld_addVehicle(LocalWorld* lw, Vehicle* v, int id);
