#include "linked_list.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


void List_init(ListHead* head) {
  head->first=0;
  head->last=0;
  head->size=0;
  //init sem too!
  sem_init(&(head->sem), 0, 1);
}

ListItem* List_find(ListHead* head, ListItem* item) {
  // linear scanning of list
  sem_wait(&(head->sem));
  ListItem* aux=head->first;
  while(aux){
    if (aux==item){
        sem_post(&(head->sem));
        return item;
  }
    aux=aux->next;
  }
  sem_post(&(head->sem));
  return 0;
}

ListItem* List_insert(ListHead* head, ListItem* prev, ListItem* item) {
    sem_wait(&(head->sem));
    if (item->next || item->prev){
        sem_post(&(head->sem));
        return 0;
    }


#ifdef _LIST_DEBUG_


    //needed to unlock sem (for the List_find below...)
    sem_post(&(head->sem));

  // we check that the element is not in the list
  ListItem* instance=List_find(head, item);
  assert(!instance);

  // we check that the previous is in the list

  if (prev) {
    ListItem* prev_instance=List_find(head, prev);
    assert(prev_instance);
  }
  // we check that the previous is in the list

    //now we need to re-acquire the sem.
    sem_wait(&(head->sem));
#endif

  ListItem* next= prev ? prev->next : head->first;
  if (prev) {
    item->prev=prev;
    prev->next=item;
  }
  if (next) {
    item->next=next;
    next->prev=item;
  }
  if (!prev)
    head->first=item;
  if(!next)
    head->last=item;
  ++head->size;
  sem_post(&(head->sem));
  return item;
}

ListItem* List_detach(ListHead* head, ListItem* item) {


#ifdef _LIST_DEBUG_
  // we check that the element is in the list
  ListItem* instance=List_find(head, item);
  assert(instance);
#endif

    sem_wait(&(head->sem));


  ListItem* prev=item->prev;
  ListItem* next=item->next;
  if (prev){
    prev->next=next;
  }
  if(next){
    next->prev=prev;
  }

  if (item==head->first){
    head->first=next;
}

  if (item==head->last){
    head->last=prev;

}

  head->size--;

  item->next=item->prev=0;

  sem_post(&(head->sem));

  return item;
}


void List_destroy(ListHead* head){
//assert empty list

sem_destroy(&(head->sem));
free(head);
}

/*
 * appends an item to a List
 */

void List_append(ListHead* head, ListItem* item){
    sem_wait(&(head->sem));
    ListItem* last = head->last;
    if(last){
        last->next = item;
        item->prev = last;
    }
    else{
        head->first = item;
    }
    head->last = item;
    head->size++;
    sem_post(&(head->sem));
}

/*
 * Prints all list items (for debug)
 */
void List_print(ListHead* head){
    sem_wait(&(head->sem));
    ListItem* aux=head->first;
    printf("list: ");
    while(aux){
    printf("%p ", aux);
    aux=aux->next;
  }
  sem_post(&(head->sem));
  printf("\n");
}
