#include <stdlib.h>

#include "random_id.h"
#include "utils.h"

RandomId* RandomId_init(void){
    RandomId* r = (RandomId*) malloc(sizeof(RandomId));
    r->id = 0;
    int ret = sem_init(&(r->sem), 0, 1);
    ERROR_HELPER(ret, "sem_init failed");
    return r;
}

int RandomId_getNext(RandomId* r){
    sem_wait(&(r->sem));
    r->id ++;
    int v = r->id;
    sem_post(&(r->sem));
    return v;
}
void RandomId_destroy(RandomId* r){
    sem_destroy(&(r->sem));
    free(r);
}
