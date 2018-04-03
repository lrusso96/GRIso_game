#include <assert.h>
#include "client_list.h"

void ClientList_init(ClientListHead* head) {
    head->first=0;
    head->last=0;
    head->size=0;
}

ClientListItem* ClientList_find(ClientListHead* head, ClientListItem* item) {
    // linear scanning of list

    ClientListItem* aux=head->first;
    while(aux){
        if (aux==item)
            return item;
        aux=aux->next;
    }
    return 0;
}

ClientListItem* ClientList_append(ClientListHead* head,ClientListItem* item) {
    if (item->next || item->prev)
        return 0;

    #ifdef _CLIENT_LIST_DEBUG_

  // we check that the element is not in the list
  ClientListItem* instance = ClientList_find(head, item);
  assert(!instance);

    #endif

    ClientListItem* last = head->last;
    if(last){
        last->next = item;
        item->prev = last;
    }
    else{
        head->first = item;
    }
    head->last = item;
    head->size++;
    return item;
}

ClientListItem* ClientList_detach(ClientListHead* head, ClientListItem* item) {

    #ifdef _CLIENT_LIST_DEBUG_

    // we check that the element is in the list
    ClientListItem* instance=List_find(head, item);
    assert(instance);

    #endif

    ClientListItem* prev=item->prev;
    ClientListItem* next=item->next;
    if (prev){
        prev->next=next;
    }
    if(next){
        next->prev=prev;
    }
    if (item==head->first)
        head->first=next;
    if (item==head->last)
        head->last=prev;
    head->size--;
    item->next=item->prev=0;

    return item;
}

void ClientList_destroy(ClientListHead* head){
    return;
}

void ClientList_print(ClientListHead* head){
    return;
}


