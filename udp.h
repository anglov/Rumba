#ifndef RUMBA_UDP_H 
#define RUMBA_UDP_H

#include <pthread.h>

#include "list.h"

//mutex - synchronizacja watkow
extern pthread_mutex_t locker;
//struktura informacji z watku UDP
typedef struct _udpInfo { int sock, port, verbose; List* broad, *adresses; } UdpInfo; 

//glowny watek dla UDP
void* udpListenDance(void* arg);
//watek wysylajacy komunikaty RUMBA cyklicznie
void* rumbaSender(void* u); 
//listeneer dla sygnalu SIGUSR1
void sig_udpListenDance(int sig);
//procedura do wyslania komunikatu msg na adresy zawarte w broadList na socket sock i port port
void sendToAll(int sock, int port, List* broadList,const char* msg);
//wypelnianie struktury UdpInfo
UdpInfo udpInfoInit(int sock, int port, List* broad, List* adresses, int verbose);

#endif
