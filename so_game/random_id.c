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
    int ret = sem_wait(&(r->sem));
    ERROR_HELPER(ret, "sem_wait failed");
    r->id ++;
    int v = r->id;
    ret = sem_post(&(r->sem));
    ERROR_HELPER(ret, "sem_post failed");
    return v;
}
void RandomId_destroy(RandomId* r){
    int ret = sem_destroy(&(r->sem));
    ERROR_HELPER(ret, "sem_destroy failed");
    free(r);
}
