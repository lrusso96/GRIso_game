#include "world.h"
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "surface.h"
#include "vehicle.h"
#include "image.h"
#include <sys/time.h>
#include <assert.h>

void World_destroy(World* w) {
  Surface_destroy(&w->ground);
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    Vehicle_destroy(v);
    item=item->next;
    free(v);
  }
}

int World_init(World* w,
	       Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step) {
  List_init(&w->vehicles);
  Image* float_image = Image_convert(surface_elevation, FLOATMONO);

  if (! float_image)
    return 0;

  Surface_fromMatrix(&w->ground,
		     (float**) float_image->row_data,
		     float_image->rows,
		     float_image->cols,
		     .5, .5, 5);
  w->ground.texture=surface_texture;
  Image_free(float_image);
  w->dt = 1;
  w->time_scale = 10;
  gettimeofday(&w->last_update, 0);
  return 1;
}

void World_update(World* w) {
  struct timeval current_time;
  gettimeofday(&current_time, 0);

  struct timeval dt;
  timersub(&current_time, &w->last_update, &dt);
  float delta = dt.tv_sec+1e-6*dt.tv_usec;
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    pthread_mutex_lock(&v->mutex);
    // decay the commands
    v->translational_force_update *= 0.999;
    v->rotational_force_update *= 0.7;
    if (! Vehicle_update(v, delta*w->time_scale)){
      Vehicle_reset(v);
    }

    pthread_mutex_unlock(&v->mutex);
    item=item->next;
  }
  w->last_update = current_time;
}

Vehicle* World_getVehicle(World* w, int vehicle_id){
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    if(v->id==vehicle_id)
      return v;
    item=item->next;
  }
  return 0;
}

Vehicle* World_addVehicle(World* w, Vehicle* v){
  assert(!World_getVehicle(w,v->id));
  return (Vehicle*)List_insert(&w->vehicles, w->vehicles.last, (ListItem*)v);
}

Vehicle* World_detachVehicle(World* w, Vehicle* v){
  return (Vehicle*)List_detach(&w->vehicles, (ListItem*)v);
}
