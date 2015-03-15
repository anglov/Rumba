#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <signal.h>
#include <pthread.h>

#include "tcp.h"
#include "list.h"
#include "fileoper.h"
#include "threadsoper.h"

#define BUFSIZE 2048

static volatile sig_atomic_t running = 1; 
static List* sharedResources;

void sig_tcpListenDance(int sig) {
    running=0;
}

void* tcpListenDance(void* arg) {
    //przetworzenie struktury argumentow
    int port = ((ArgListIntInt*)arg)->in1;
    int verbose = ((ArgListIntInt*)arg)->in2;
    List* ipadr = ((ArgListIntInt*)arg)->list1;
    struct sockaddr_in myaddr;
    int fd;
    //tworzenie socketow
    for(ListElem *ptr=ipadr->head;ptr;ptr=ptr->next) {
        fd=socket(AF_INET,SOCK_STREAM,0);
        memset(&myaddr,'0',sizeof(myaddr));
        myaddr.sin_family=AF_INET;
        myaddr.sin_port=htons(port);
        myaddr.sin_addr.s_addr=((struct sockaddr_in *)ptr->d)->sin_addr.s_addr;
        if(fd==-1) {
            perror("Cannot create TCP socket\n");
            exit(1);
        }
        socklen_t len=sizeof(myaddr);
        if(bind(fd,( struct sockaddr*)&myaddr,len)==-1) {
            perror("Bind TCP socket failed");
            exit(1); 
        }
        if(listen(fd,10)==-1) {
            perror("Couldn't listen TCP socket");
            exit(1);
        }
    }
    
    //alokacja listy udostepnianych plikow 
    sharedResources = malloc(sizeof(List));
    if (!sharedResources) {
        perror("Cannot allocate memory");
        exit(1);
    }
    listInit(sharedResources);
    //wypelnianie listy
    if(readShared(sharedResources,"shared.txt")<0) 
        printf("No file is shared\n");
    //rozpoczecie nasluchiwania
    while(running){
        int sockid = accept(fd,0,0);
        if(running){
            verbose?printf("TCP S: Client want to connect: socket %d\n", sockid):0;
            ArgListIntInt* argChild = malloc(sizeof(ArgListIntInt));
            if (!argChild){
                perror("Cannot allocate memory");
                exit(1);
            }
            fillArgStruct( argChild,NULL, NULL, NULL, sockid,  verbose); 
            //tworzenie watku typu detached do realizacja poszczegolnych polaczen
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_t client;
            int err = pthread_create(&client,&attr,tcpHandler, argChild);
            if (err<0){
                perror("Cannot create TCP Handler");
            }
        }
    }
    //procedury konczace
    close(fd);
    deleteAllAllocate(sharedResources);
    free(sharedResources);
    return NULL;
}

void* tcpHandler(void* arg) {
    //inicjalizacja zmiennych oraz przetworzenie arrgumentow
    int bye = 0;
    int k=1;
    int sockid = ((ArgListIntInt*)arg)->in1;
    int verbose = ((ArgListIntInt*)arg)->in2;
    char buf[BUFSIZE+1];
    verbose?printf("TCP S: Client connected on: socket %d\n", sockid):0;
    while(k) {
        //czekanie na wiadomosc
        int recvlen = recv(sockid, buf, BUFSIZE, 0);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            verbose?printf("TCP S: received message: %s", buf):0;
        }
        //obsluga polecenia BYE
        if (!strcmp(buf, "BYE")) {
            bye = 1;
            k = 0;
        }
        //obsluga polecenia GETLIST
        else if (!strcmp(buf, "GETLIST\n")) {
            send(sockid, "OK\n", sizeof("OK\n"),0);
            if(sendList(sockid, sharedResources))
                send(sockid, "ERROR: sending list is not possible\n", 
                     sizeof("ERROR: sending list is not possible\n"),0);
            bye=1;
            k=0;
        }
        //obsluga polecenia GET szczegoly w specyfikacji protokolu
        else if (!strcmp(buf, "GET\n")) {
            send(sockid, "OK\n", sizeof("OK\n"),0);
            recvlen = recv(sockid, buf, BUFSIZE, 0);
            if (!recvlen > 0) k=0;
            else {
                buf[recvlen] = 0;
                verbose?printf("TCP S: Received message: %s\n", buf):0;
                int fileNum = atoi(buf);
                //jesli BYE
                if(!strcmp(buf,"BYE\n")){
                        bye = 1;
                        k = 0;
                }
                if(fileNum==0 || fileNum>sharedResources->count)
                    send(sockid, "ERROR: invalid request\n", 
                         sizeof("ERROR: invalid request\n"),0);
                else {
                    send(sockid, "OK\n", sizeof("OK\n"),0);
                    char* path =  (char*)getN(sharedResources,fileNum)->d;
                    char* filename = getFilename(path);
                    send(sockid, filename, strlen(filename),0);
                    free(filename);
                    recvlen = recv(sockid, buf, BUFSIZE, 0);
                    buf[recvlen] = '\0';
                    verbose?printf("TCP S: Received message: %s", buf):0;
                    //jesli BYE
                    if(!strcmp(buf,"BYE\n")){
                        bye = 1;
                        k = 0;
                    }
                    //jesli nie OK
                    else if(strcmp(buf,"OK\n")) {
                        printf("%s", buf);
                        k = 0;
                    }
                    //jesli OK
                    else if(!strcmp(buf,"OK\n")) {
                        if(sendFile(sockid,path)) 
                            send(sockid, "ERROR: while proceding file\n", 
                                sizeof("ERROR: while proceding file\n"),0);
                        k=0;
                        bye = 1;
                    }
                }
            }
        }
        //pozostale nie zaimplementowane
        else 
            if(send(sockid, "Not implemented\n", 
                sizeof("Not implemented\n"), 0)==-1) k=0;
    }
    //procedury konczace
    verbose?printf("TCP S: Closing connection\n"):0;
    if(!bye) send(sockid, "BYE\n", sizeof("BYE\n"), 0);
    free(arg);
    close(sockid);
    return NULL;
}
