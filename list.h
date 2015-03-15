#ifndef LIST_H
#define LIST_H

#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <netdb.h>

//definicje typow
typedef struct _LE {void* d; struct _LE *next, *prev; } ListElem;
typedef struct _List {int count; ListElem *head, *tail, *current;} List;


//inicjalizuje liste
void listInit ( List *pL );
//tworzy nowy element (bez wstawiania go do listy)
ListElem* newElem (void* D );
//tworzy i alokuje nowy element sockaddr_in (bez wstawiania go do listy)
ListElem* newElemSockAddr ( struct sockaddr_in* elem );
//tworzy i alokuje nowy element int (bez wstawiania go do listy)
ListElem* newElemInt ( int* elem );
//tworzy i alokuje nowy element char* (bez wstawiania go do listy)
ListElem* newElemString ( char* elem );
//dodaje element na koncu listy
void addEnd ( List *pL, ListElem *new );
//usuwa element
void removeElem( List *pL,struct sockaddr_in* elem);
//usuwa elment i dealokuje pamięć
void removeElemAllocate( List *pL,struct sockaddr_in* elem);
//usuwa wszystkie elementy
void deleteAll ( List *pL );
//usuwa wszystkie elementy z listy z zaalokowaną pamięcią na elementy
void deleteAllAllocate(List *pL);
//przeszukuje liste sockaddr_in jak znajdzie elem, jesli nie 0 TODO: make it generic
ListElem* find( List *pL,struct sockaddr_in* elem);
//przeszukuje liste char* jak znajdzie elem, jesli nie 0 TODO: make it generic
ListElem* findString (List *pL, char* elem);
//wyswietla adresy ip z sockaddr_in TODO: make it generic
void printAll( List *pL);
//sprawdza czy lista jest pusta
int isEmpty( List *pL);
//pobiera Nty element z listy
ListElem * getN( List *pL, int N);
#endif
