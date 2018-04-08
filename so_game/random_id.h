#pragma once

#include <semaphore.h>

typedef struct RandomId{
    int id;
    sem_t sem;
}RandomId;

RandomId* RandomId_init(void);
int RandomId_getNext(RandomId* r);
void RandomId_destroy(RandomId* r);
