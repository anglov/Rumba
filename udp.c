#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#include "udp.h"
#include "threadsoper.h"

#define BUFSIZE 2048

static volatile sig_atomic_t running = 1; 

void* rumbaSender(void* u) {
    //inicjalizacja
    int sock = ((UdpInfo*)u)->sock;
    int port = ((UdpInfo*)u)->port;
    List* broad = ((UdpInfo*)u)->broad;
    List* adresses = ((UdpInfo*)u)->adresses;
    int verbose = ((UdpInfo*)u)->verbose;
    while(running) {
        //kasowanie listy obecnosci
        pthread_mutex_lock(&locker);
        deleteAll(adresses);
        pthread_mutex_unlock(&locker);
        //wyslanie przywitania
        verbose?printf("UDP: Broadcasting RUMBA\n"):0;
        sendToAll(sock, port, broad, "RUMBA");
        //odczekaj
        if(running)sleep(60);
    }
    //wyslanie pozegnania
    verbose?printf("UDP: Time to say GoodBYE\n"):0;
    sendToAll(sock, port, broad,"BYE");
    return NULL;
}

void sig_udpListenDance(int sig) {
    running=0;
}

void sendToAll(int sock, int port, List* broadList,const char* msg) {
    struct sockaddr_in broadaddr;
    memset((char *)&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_port = htons(port);
    for(ListElem *ptr=broadList->head;ptr;ptr=ptr->next) {
        broadaddr.sin_addr.s_addr=((struct sockaddr_in *)ptr->d)->sin_addr.s_addr;
        socklen_t addr_len = sizeof(broadaddr);
        sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&broadaddr, addr_len);
    }
}

void* udpListenDance(void* arg) {
    //inicjalizacja
    List* adresses = ((ArgListIntInt*)arg)->list1;
    List* ipadr = ((ArgListIntInt*)arg)->list2;
    List* broadadr = ((ArgListIntInt*)arg)->list3;

    int port = ((ArgListIntInt*)arg)->in1;
    int verbose = ((ArgListIntInt*)arg)->in2;
    struct sockaddr_in myaddr, remoteaddr;
    UdpInfo uinfo;
    pthread_t presenceOsc; 
    socklen_t addr_len;
    int recvlen, fd;
    unsigned char buf[BUFSIZE+1];
    //tworzenie gniazda
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Cannot create UDP socket\n");
        exit(1);
    }
    int optval = 1;
    int response = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
    if (response < 0){
        perror("Setsockopt failed (for broadcast)\n");
        exit(1);
    }
    
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("Bind UDP socket failed");
        exit(1);
    }
    uinfo=udpInfoInit(fd, port, broadadr, adresses, verbose); 
    //utworzenie watku do cyklicznego wysylania obecnosci
    pthread_create(&presenceOsc, NULL, rumbaSender, &uinfo);
    
    while(running){
        memset((char *)&remoteaddr, 0, sizeof(remoteaddr));
        addr_len = sizeof(remoteaddr);
        //pobieranie wiadomosci
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remoteaddr, &addr_len);
        if(running){
            // jesli prawidlowa
            if (recvlen > 0) {
                buf[recvlen] = 0;
                verbose?printf("UDP: Received message: \"%s\"\n", buf):0;
            }
            //jesli RUMBA i nie wlasny
            if(!strcmp((const char*)buf, "RUMBA") && !find(ipadr,&remoteaddr)) {
                char temp[30];
                inet_ntop(AF_INET,&(remoteaddr.sin_addr),temp,sizeof(temp));
                verbose?printf("UDP: Remote host: %s Sending response\n", temp):0;
                pthread_mutex_lock(&locker);
                //jesli jeszcze nie na liscie 
                if(!find(adresses, &remoteaddr)) {
                    verbose?printf("UDP: Remembered: %s\n",temp):0;
                    addEnd(adresses,newElemSockAddr(&remoteaddr));
                }
                pthread_mutex_unlock(&locker);
                sendto(fd, "DANCE", sizeof("DANCE"), 0, (struct sockaddr *)&remoteaddr, addr_len);
            }
            //jesli DANCE
            else if(!strcmp((const char*)buf, "DANCE")) {
                char temp[30];
                inet_ntop(AF_INET,&(remoteaddr.sin_addr),temp,sizeof(temp));
                pthread_mutex_lock(&locker);
                //jesli jeszcze nie na liscie
                if(!find(adresses, &remoteaddr)) {
                    verbose?printf("UDP: Remembered: %s\n",temp):0;
                    addEnd(adresses,newElemSockAddr(&remoteaddr));
                }
                pthread_mutex_unlock(&locker);
            }
            //jesli BYE
            else if(!strcmp((const char*)buf, "BYE")) {
                verbose?printf("UDP: Time to end dance :(\n"):0;
                pthread_mutex_lock(&locker);
                removeElemAllocate(adresses,&remoteaddr);
                pthread_mutex_unlock(&locker);
            }

        }
    }
    //procedury kończące
    signal(SIGUSR1,sig_nop);
    closeThread(&presenceOsc);
    close(fd);
    return NULL;
}


UdpInfo udpInfoInit(int sock, int port, List* broad, List* adresses, int verbose)  {
    UdpInfo u;
    u.sock=sock;
    u.port=port;
    u.broad=broad;
    u.adresses=adresses;
    u.verbose=verbose;
    return u;
}
