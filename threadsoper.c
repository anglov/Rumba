#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>

#include "threadsoper.h"

void fillArgStruct(ArgListIntInt* arg, List* list1, List* list2, List* list3, int in1, int in2){
    arg->list1 = list1;
    arg->list2 = list2;
    arg->list3 = list3;
    arg->in1 = in1;
    arg->in2 = in2;
}

void closeThread(pthread_t* thread) {
    pthread_kill(*thread,SIGUSR1);
    pthread_join(*thread, NULL);
}

void sig_nop(int sig){
    return;
}

void ipbroadcastAdresses(List *ipList,List *broadList ) {
    struct ifaddrs* ifap, *current;
    getifaddrs(&ifap);

    if (!ifap) {
        perror("No interfaces found\n");
        exit(1);
    }
    for(current=ifap;current;current = current->ifa_next){
        if (current->ifa_addr->sa_family==AF_INET && strcmp(current->ifa_name,"lo")){
            struct sockaddr_in* ip = malloc(sizeof(struct sockaddr_in ));
            *ip = *(struct sockaddr_in *) current->ifa_addr;
            struct sockaddr_in* broad = malloc(sizeof(struct sockaddr_in ));
            *broad = *(struct sockaddr_in *) current->ifa_dstaddr;
            addEnd(ipList,newElem(ip));
            addEnd(broadList,newElem(broad));
        }
    }
    freeifaddrs(ifap);
}
