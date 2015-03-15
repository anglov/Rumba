#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include "tcp.h"
#include "udp.h"
#include "list.h"
#include "threadsoper.h"
#include "tcpclient.h"
#include "fileoper.h"


#define BUFSIZE 2048

pthread_mutex_t locker;

void showHelp();
void runtimeHelp();

int main(int argc, char **argv) {
    //wartosci domyslne parametrow
    int verbose = 0;
    int tcpPort = 34000;
    int udpPort = 9000;
    int c;
    //obsluga parametrow wejsciowych - biblioteka getopt
    while ((c = getopt (argc, argv, "a:ht:u:v")) != -1) {
        switch (c)
        {
            case 'a':
                addToEnd("shared.txt", optarg);
                return 0;
            case 'h':
                showHelp();
                return 0;
            case 't':
                tcpPort = atoi(optarg);
                break;
            case 'u':
                udpPort = atoi(optarg);
                break;                
            case 'v':
                verbose = 1;
                break;
            case '?':
                if (optopt == 't' || optopt == 'u')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr,"Unknown option '-%c'.\n", optopt);
                else
                    fprintf (stderr,"Unknown option character '%c'.\n", optopt);
                return 1;
            default:
                abort ();
        }
    }
    //argument "wolny" - folder przechowywania plikow 
    if (optind>=argc) {
        fprintf (stderr,"You need to specify destination directory\n");
        return 1;
    }
    char* destDir = argv[optind];
    struct stat st;
    lstat(destDir, &st);
    //czy to katalog
    if(!S_ISDIR(st.st_mode)){
        fprintf (stderr,"%s is not a directory\n", destDir);
        return 1;
    }
    //czy prawo do zapisu
    if(access(destDir, W_OK)) {
        perror("Destination directory error");
        return 1;
    }
    
    //tworzenie list 
    int err=0;
    List* adresses;
    adresses = malloc(sizeof(List));
    if (!adresses){
        perror("Cannot allocate memory");
        return 1;
    }
    listInit(adresses);
    
    List* ipadr = malloc(sizeof(List));
    if (!ipadr){
        perror("Cannot allocate memory");
        return 1;
    }
    List* broadadr = malloc(sizeof(List));
    if (!broadadr){
        perror("Cannot allocate memory");
        return 1;
    }

    listInit(ipadr);
    listInit(broadadr);
    //pobranie konfiguracji sieci
    ipbroadcastAdresses(ipadr, broadadr);
    
    //zamaskowanie sygnalu SIGPIPE - dopuszczamy pisanie do zamknietego socketa
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGPIPE);
    sigprocmask (SIG_BLOCK, &mask, NULL);
    
    //inicjalizacja mutexa - potrzebne do sekcji krytycznej
    err = pthread_mutex_init(&locker, NULL);
    if (err<0){
        perror("Cannot create locker");
        return 1;
    }
    //tworenie watkow UDP i TCP
    pthread_t udpListener, tcpListener;
    ArgListIntInt tcpArg, udpArg;
    fillArgStruct(&udpArg, adresses, ipadr, broadadr, udpPort, verbose);
    fillArgStruct(&tcpArg, ipadr, NULL, NULL, tcpPort, verbose);
    err = pthread_create(&udpListener,NULL,udpListenDance, &udpArg);
    if (err<0){
        perror("Cannot create UDP listener");
        return 1;
    }

    err = pthread_create(&tcpListener,NULL,tcpListenDance, &tcpArg);
    if (err<0){
        perror("Cannot create TCP listener");
        return 1;
    }
    
    //obsluga polecen uzytkownika
    int k=1;
    while(k){
        //bezpieczne odczytywanie stdin
        char buff[200];
        int readed = read(STDIN_FILENO, buff, sizeof(buff)-1);
        buff[readed-1] = 0;
        char *words[readed/2], *ptr;
        for(int i=0;i<readed/2;++i) words[i]=0;
        //wlasna implementacja tokenizacji stringa
        int params=1;
        ptr=words[0]=buff;
        while ((ptr = strchr(ptr,' '))) {
            *ptr='\0';
            words[params]=++ptr;
            ++params;
        }
        //polecenie shared (s)
        if(!strcmp(words[0],"shared") || !strcmp(words[0],"s")) { 
            //sprawdzenie liczby parametrow
            if(params<2) 
                printf("Too few parameters. Type 'help' to get information\n");
            else {
                //sprawdzenie poprawnosci adresu IP 
                struct sockaddr_in remote;
                err=inet_pton(AF_INET, words[1], &remote.sin_addr);
                if(err==0)
                    printf("%s is not a valid IP adress\n",words[1]);
                else if (err==-1)
                    perror("IP Conversion Error");
                else {
                    //sprawdzenie, czy zadany adres na liscie obslugujacych i ew. nawiazanie polaczenia
                    pthread_mutex_lock(&locker);
                    if(find(adresses,&remote)) {
                        pthread_mutex_unlock(&locker);
                        getShared(&remote, tcpPort, verbose);
                    }
                    else {
                        pthread_mutex_unlock(&locker);
                        printf("%s don't want to dance with us\n", words[1]);
                    }
                }
            }
        }
        //polecenie get (g)
        else if(!strcmp(words[0],"get") || !strcmp(words[0],"g")) { 
            //sprawdzaie liczby parametrow
            if(params<3) 
                printf("Too few parameters. Type 'help' to get information\n");
            else {
                //sprawdzanie poprawnosci adresu IP
                struct sockaddr_in remote;
                err=inet_pton(AF_INET, words[1], &remote.sin_addr);
                if(err==0)
                    printf("%s is not a valid IP adress\n",words[1]);
                else if (err==-1)
                    perror("IP Conversion Error");
                else {
                    //sprawdzenie, czy zadany adres na liscie obslugujacych i ew. nawiazanie polaczenia
                    pthread_mutex_lock(&locker);
                    if(find(adresses,&remote)) {
                        pthread_mutex_unlock(&locker);
                        getFile(&remote, tcpPort, words[2], destDir, verbose);
                    }
                    else {
                        pthread_mutex_unlock(&locker);
                        printf("%s don't want to dance with us\n", words[1]);
                    }
                }
            }
        }

        //polecenie list (l)
        else if(!strcmp(words[0],"list") || !strcmp(words[0],"l")) { 
            //wyswietlanie adresow w sekcji krytycznej
            pthread_mutex_lock(&locker);
            printAll(adresses);
            pthread_mutex_unlock(&locker);
        }
        
        //polecenie exit (e)
        else if(!strcmp(words[0],"exit") || !strcmp(words[0],"e")) { 
            //kończenie watkow
            signal(SIGUSR1,sig_udpListenDance);
            closeThread(&udpListener);
            signal(SIGUSR1,sig_tcpListenDance);
            closeThread(&tcpListener);
            k=0;
        }

        //polecenie help (h)
        else if(!strcmp(words[0],"help") || !strcmp(words[0],"h")) { 
            runtimeHelp();
        }
        
        //nieprawidlowy parametr
        else { 
            printf("'%s' is not a dancer command. Type 'help' to get information\n", words[0]);
        }

    }
    //zwalnianie pamieci
    deleteAllAllocate(adresses);
    free(adresses);
    deleteAllAllocate(ipadr);
    free(ipadr);
    deleteAllAllocate(broadadr);
    free(broadadr);
    return 0;
}

//pomoc ogolna
void showHelp() {
   printf("Dancer 0.00001, Rumba(v.0.00001) sharing protocol client\n"
          "Syntax: dancer [OPTIONS] destination\n"
          "\n"
          "destination - directory, where you want to save downloaded files\n"
          "\n"
          "Running:\n"
          "-a PATH\tAdd PATH to shared resources\n"
          "-h\tShow this help\n"
          "-t\tSpecify TCP port. Default: 24000\n"
          "-u\tSpecify UDP port. Default: 9000\n"
          "-v\tSwitch to verbose mode (it's really verbose)\n"
          "\n"
          "Runtime:\n");
   runtimeHelp();
}

//pomoc po uruchomieniu
void runtimeHelp() {
    printf("Program handle options:\n"
           "e\texit\t\tEnd the program\n"
           "g\tget IP FILENUM\tFrom IP (see list) get file with FILENUM (see shared)\n"
           "h\thelp\t\tShow this info\n"
           "l\tlist\t\tShow list of computers, which use awesome Rumba protocol\n"
           "s\tshared IP\tGet list of shared resources from IP (see list)\n");
}
