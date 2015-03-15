#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <arpa/inet.h>
#include "list.h"


void listInit ( List *pL ) {
    pL->head = pL->tail = pL->current = NULL;
    pL->count=0;
}

ListElem* newElem (void* D ){
    ListElem *new = (ListElem*) malloc( sizeof( ListElem ) );
    if (!new){
        perror("Cannot allocate memory");
        exit(1);
    }
    new->d = D;
    new->next = new->prev = NULL;
    return new;
}

ListElem* newElemSockAddr ( struct sockaddr_in* elem ){
    ListElem *new = (ListElem*) malloc( sizeof( ListElem ) );
    if (!new){
        perror("Cannot allocate memory");
        exit(1);
    }
    struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
    if (!addr){
        perror("Cannot allocate memory");
        exit(1);
    }

    *addr = *elem;
    new->d = addr;
    new->next = new->prev = NULL;
    return new;
}

ListElem* newElemInt ( int* elem ){
    ListElem *new = (ListElem*) malloc( sizeof( ListElem ) );
    if (!new){
        perror("Cannot allocate memory");
        exit(1);
    }
    int* addr = malloc(sizeof(elem));
    if (!addr){
        perror("Cannot allocate memory");
        exit(1);
    }

    *addr = *elem;
    new->d = addr;
    new->next = new->prev = NULL;
    return new;
}

ListElem* newElemString ( char* elem ){
    ListElem *new = (ListElem*) malloc( sizeof( ListElem ) );
    if (!new){
        perror("Cannot allocate memory");
        exit(1);
    }
    char* addr = malloc(strlen(elem)*sizeof(char*));
    strcpy(addr,elem);
    new->d = addr;
    new->next = new->prev = NULL;
    return new;
}

void addEnd ( List *pL, ListElem *new )
{
    if ( pL->tail ) pL->tail->next = new;
    else pL->head = new;
    new->prev = pL->tail;
    new->next = NULL;
    pL->tail = new;
    pL->count=pL->count+1;
}

void deleteAll ( List *pL )
{
    ListElem *tmp;
    while ( pL->head ) {
        tmp = pL->head->next;
        free( pL->head );
        pL->head = tmp;
    }
    listInit(pL);
}

void deleteAllAllocate(List *pL) {
    ListElem *tmp;
    while ( pL->head )
    {
        tmp = pL->head->next;
        free( pL->head->d );
        free( pL->head );
        pL->head = tmp;
    }
    listInit(pL);
}

void removeElem( List *pL,struct sockaddr_in* elem) {
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next)
        if (((struct sockaddr_in*)ptr->d)->sin_addr.s_addr == elem->sin_addr.s_addr) {
            if(ptr->prev) ptr->prev->next = ptr->next;
            else pL->head = ptr->next; 
            if(ptr->next) ptr->next->prev = ptr->prev;
            else pL->tail = ptr->prev;
            free(ptr);
            pL->count=pL->count-1;
            return;
        }
    return;
}

void removeElemAllocate( List *pL,struct sockaddr_in* elem) {
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next)
        if (((struct sockaddr_in*)ptr->d)->sin_addr.s_addr == elem->sin_addr.s_addr) {
            if(ptr->prev) ptr->prev->next = ptr->next;
            else pL->head = ptr->next; 
            if(ptr->next) ptr->next->prev = ptr->prev;
            else pL->tail = ptr->prev;
            free(ptr->d);
            free(ptr);
            pL->count=pL->count-1;
            return;
        }
    return;
}

ListElem * findString (List *pL, char* elem) {
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next)
        if (!strcmp((char*)ptr->d, elem)) return ptr;
    return NULL;
}

ListElem * find( List *pL, struct sockaddr_in* elem) {
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next)
        if (((struct sockaddr_in*)ptr->d)->sin_addr.s_addr == elem->sin_addr.s_addr) return ptr;
    return NULL;
}

ListElem * getN( List *pL, int N) {
    int i=1;
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next)
        if(i++==N) return ptr;
    return NULL;
}

void printAll( List *pL){
    char buff[20];
    for(ListElem *ptr=pL->head;ptr;ptr=ptr->next){
        void* addr = &((struct sockaddr_in*)ptr->d)->sin_addr;
        inet_ntop(AF_INET,addr,buff,sizeof(buff));
        printf("%s\n",buff);
    }
    return;    
}

int isEmpty( List *pL) {
    if(pL->head==NULL) return 1;
    else return 0;
}
