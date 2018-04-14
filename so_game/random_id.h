#pragma once

#include <semaphore.h>

/*
 * This handles in critical section the unique_id getter method.
 *
 * For now it only cares of updating (++) the last id. The id is unique per connection
 * and is not recycled
 *
 */

typedef struct RandomId{
    int id;
    sem_t sem;
}RandomId;

RandomId* RandomId_init(void);
int RandomId_getNext(RandomId* r);
void RandomId_destroy(RandomId* r);
