#ifndef RUMBA_TCP_H 
#define RUMBA_TCP_H

//watek glowny TCP
void* tcpListenDance( void * arg);
//listeneer dla sygnalu SIGUSR1
void sig_tcpListenDance(int sig);
//realizacja polaczen
void* tcpHandler(void* sockid);

#endif
