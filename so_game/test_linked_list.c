#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include "linked_list.h"

ListHead* l1;
ListItem* i1, *i2;

int main(int argc, char const * argv[]){

    l1 = (ListHead*) malloc(sizeof(ListHead));

    List_init(l1);

    i1 = (ListItem*) malloc(sizeof(ListItem));
    i2 = (ListItem*) malloc(sizeof(ListItem));


    List_append(l1, i1);
    List_print(l1);

    List_append(l1, i2);
    List_print(l1);

    List_detach(l1, i1);
    List_print(l1);

    List_detach(l1, i2);
    List_print(l1);

    free(i1);
    free(i2);

    List_destroy(l1);

}
