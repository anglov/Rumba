#ifndef THREAD_OPER_H 
#define THREAD_OPER_H 

#include <pthread.h>
#include "list.h"

//struktura dla argumentow watkow 
typedef struct _ListIntInt { List *list1, *list2, *list3; int in1, in2; } ArgListIntInt;

//wypelnianie struktury argumentow
void fillArgStruct(ArgListIntInt* arg, List* list1, List* list2, List* list3, int in1, int in2);
//procedury konczace watkow – wyslanie SIGUSR oraz join
void closeThread(pthread_t* thread);
//procedura braku reakcji na sygnal
void sig_nop(int sig);
//pobranie listy adresow ip oraz broadcast z konfiguracji sieci
void ipbroadcastAdresses(List *ipList, List *broadList);

#endif
