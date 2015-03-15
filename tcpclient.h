#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

//przygotowanie polaczenia TCP dla klienta 
int tcpConnectPrepare(struct sockaddr_in* remote, int tcpport, int verbose);
//realizacja pobierania listy plikow
void getShared(struct sockaddr_in* remote, int tcpport, int verbose);
//realizacja pobierania plikow
void getFile(struct sockaddr_in* remote, int tcpport, char* filenum, char* dir, int verbose);

#endif
