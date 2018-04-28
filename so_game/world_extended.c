#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "image.h"
#include "utils.h"
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

    we->max_id = 128;
    we->ids = (bool*) calloc(128, sizeof(bool));
    we->with_texture = (bool*) calloc(128, sizeof(bool));
    return we;

}
void WorldExtended_destroy(WorldExtended* we){

    World* w = we->w;

    ListItem* item=w->vehicles.first;
    while(item){
        Vehicle* v=(Vehicle*)item;
        item = item->next;
        WorldExtended_detachVehicle(we, v->id);

  }

    //when world is empty
    World_destroy(we->w);
    free(we->w);
    free(we->ids);
    free(we->with_texture);
    free(we);
}

int WorldExtended_addVehicle(WorldExtended* we, Vehicle* v){
    World_addVehicle(we->w, v);
    int s = we->w->vehicles.size;
    we->online_users = s;
    int m = we->max_id;
    int id = v->id;
    if(id>m){
        we->ids = realloc(we->ids, m*2);
        we->with_texture = realloc(we->with_texture, m*2);
    }

    we->ids[id] = true;
    we->with_texture[id] = true;

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
    we->ids[id] = false;
    we->with_texture[id] = false;
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


void WorldExtended_vehicleUpdatePacket_init(WorldExtended* we, VehicleUpdatePacket* p, Vehicle* v){

    int ret;
    ListHead* lh = &(we->w->vehicles);
    //we need to do this in critical section!
    ret = sem_wait(&(lh->sem));
    ERROR_HELPER(ret, "sem_wait failed");

    ListItem* item = lh->first;
    while(item){
        Vehicle* vv = (Vehicle*)item;

        if(vv==v){
	pthread_mutex_lock(&v->mutex);
            p->x = v->x;
            p->y = v->y;
            p->theta = v->theta;
            p->rotational_force = v->rotational_force_update;
            p->translational_force = v->translational_force_update;
	    pthread_mutex_unlock(&v->mutex);
            ret = sem_post(&(lh->sem));
            ERROR_HELPER(ret, "sem_post failed");
            return;
        }

        item=item->next;
    }
    ret = sem_post(&(lh->sem));
    ERROR_HELPER(ret, "sem_post failed");

}


void WorldExtended_getVehicleForcesUpdate(WorldExtended* we, Vehicle* v, float* tf, float* rf){

    int ret;
    ListHead* lh = &(we->w->vehicles);
    //we need to do this in critical section!
    ret = sem_wait(&(lh->sem));
    ERROR_HELPER(ret, "sem_wait failed");

    ListItem* item = lh->first;
    while(item){
        Vehicle* vv = (Vehicle*)item;

        if(vv==v){
            *tf=v->translational_force_update;
            *rf=v->rotational_force_update;
            ret = sem_post(&(lh->sem));
            ERROR_HELPER(ret, "sem_post failed");
            return;
        }
        item=item->next;
    }
    ret = sem_post(&(lh->sem));
    ERROR_HELPER(ret, "sem_post failed");
}

void WorldExtended_getVehicleXYTPlus(WorldExtended* we, Vehicle* v, float* x, float* y, float* t, float* rf, float* tf){

    int ret;
    ListHead* lh = &(we->w->vehicles);

    //we need to do this in critical section!
    ret = sem_wait(&(lh->sem));
    ERROR_HELPER(ret, "sem_wait failed");

    ListItem* item = lh->first;
    while(item){
        Vehicle* vv = (Vehicle*)item;
	pthread_mutex_lock(&vv->mutex);
        if(vv==v){
            *x=v->x;
            *y=v->y;
            *t=v->theta;
            *rf = v->rotational_force_update;
            *tf = v->translational_force_update;
            ret = sem_post(&(lh->sem));
            ERROR_HELPER(ret, "sem_post failed");
            return;
        }
	pthread_mutex_unlock(&v->mutex);
        item=item->next;
    }
    ret = sem_post(&(lh->sem));
    ERROR_HELPER(ret, "sem_post failed");
}


void WorldExtended_setVehicleXYTPlus(WorldExtended* we, int id, float x, float y, float t, float rf, float tf){

    int ret;
    ListHead* lh = &(we->w->vehicles);

    //we need to do this in critical section!
    ret = sem_wait(&(lh->sem));
    ERROR_HELPER(ret, "sem_wait failed");

    ListItem* item = lh->first;
    while(item){
        Vehicle* v = (Vehicle*)item;
	
        if(v->id==id){
	pthread_mutex_lock(&v->mutex);
            v->x = x;
            v->y = y;
            v->theta = t;
            v->rotational_force_update = rf;
            v->translational_force_update = tf;
	pthread_mutex_unlock(&v->mutex);
            ret = sem_post(&(lh->sem));
            ERROR_HELPER(ret, "sem_post failed");
            return;
        }
	
        item=item->next;
    }
    ret = sem_post(&(lh->sem));
    ERROR_HELPER(ret, "sem_post failed");
}


int WorldExtended_HasIdAndTexture(WorldExtended* we, int id){

    /*
    if(we->max_id < id)
        return -1;
    if(we->ids[id] == true && we->ids == false)
        return -1;
    return 0;
    *
    */
    if(we->ids[id] == true && we->ids[id] == true)
        return 0;
    return -1;
}















