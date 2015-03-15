#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "tcpclient.h"
#include "fileoper.h"

#define BUFSIZE 2048

int tcpConnectPrepare(struct sockaddr_in* remote, int tcpport, int verbose) {
    //prrzygotowywanie socketa
    int  sock=socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr = remote->sin_addr;
    dest.sin_port = htons(tcpport);

    if(connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr))){
        perror("Error while connect to server");
        return 0;
    }
    verbose?printf("TCP C: connection to server success\n"):0;
    return sock;
}


void getShared(struct sockaddr_in* remote, int tcpport, int verbose) {
    char buffer[BUFSIZE + 1];
    int sock;
    //przygotowanie połaczenia
    if (!(sock = tcpConnectPrepare(remote, tcpport, verbose))) {
        printf("Error: cannot connect to server");
        return;
    }
    //wyslanie polecenia (patrz opis protokolu)
    send(sock, "GETLIST\n", strlen("GETLIST\n"), 0);
    //oczekiwanie na odpowiedz
    int len = recv(sock, buffer, BUFSIZE, 0);
    buffer[len] = '\0';
    verbose?printf("TCP C: received message: %s", buffer):0;
    //jesli nie OK
    if(strcmp(buffer,"OK\n"))
        printf("Error: %s\n", buffer);
    //jesli OK
    else {
        len = recv(sock, buffer, BUFSIZE, 0);
        while(len){
            if(!strcmp(buffer,"BYE\n")) {
                close(sock);
                return;
            }
            printf("%s",buffer);
            len = recv(sock, buffer, BUFSIZE, 0);
            buffer[len] = '\0';
        }
    }
    //pozegnanie
    send(sock, "BYE\n", strlen("BYE\n"), 0);
    verbose?printf("TCP C: closing connection\n"):0;
    close(sock);
}

void getFile(struct sockaddr_in* remote, int tcpport, char* filenum, char* dir, int verbose) {
    char buffer[BUFSIZE + 1];
    int sock, len;
    int bye = 0;
    if (!(sock = tcpConnectPrepare(remote, tcpport, verbose))) {
        printf("Error: cannot connect to server");
        return;
    }
    //wyslanie polecenia (patrz opis protokolu)
    send(sock, "GET\n", strlen("GET\n"), 0);
    //ocekiwanie na odpowiedz
    len = recv(sock, buffer, BUFSIZE, 0);
    buffer[len] = '\0';
    verbose?printf("TCP C: received message: %s", buffer):0;
    //jesli BYE
    if(!strcmp(buffer,"BYE\n")) bye = 1;
    //jesli nie OK
    else if(strcmp(buffer,"OK\n"))
        printf("%s", buffer);
    //jesli OK
    else {
        //wyslanie id pliku
        verbose?printf("TCP C: sending ID of file: %s\n", filenum):0;
        send(sock, filenum, strlen(filenum), 0);
        //czekanie na odpowiedz
        len = recv(sock, buffer, BUFSIZE, 0);
        buffer[len] = '\0';
        verbose?printf("TCP C: received message: %s", buffer):0;
        //jesli BYE
        if(!strcmp(buffer,"BYE\n")) bye = 1;
        //jesli nie OK
        else if(strcmp(buffer,"OK\n"))
            printf("%s", buffer);
        //jesli OK
        else {
            //oczekiwanie na nazwe pliku
            len = recv(sock, buffer, BUFSIZE, 0);
            buffer[len] = '\0';
            verbose?printf("TCP C: received message: %s\n", buffer):0;
            //pobranie sciezki do zapisu pliku
            char* path = getPath(dir, buffer);
            int get=1;
            //jesli plik istnieje
            if(!access(path, F_OK)) {
                int k=1;
                //czy chcesz nadpisac
                while(k){
                    printf("%s exists. Do you want to overwrite it? (y/n)\n", buffer);
                    char tmp[BUFSIZE];
                    len = read(STDIN_FILENO, tmp, sizeof(tmp)-1);
                    tmp[len-1] = 0;
                    if (tmp[0]=='y') {
                        k=0;
                    }
                    else if (tmp[0]=='n') {
                        k=0;
                        get=0;
                    }
                }
            }
            if(get){
                //jesli wszystko w porzadku wyslanie ok
                send(sock, "OK\n", strlen("OK\n"), 0);
                verbose?printf("TCP C: start reading\n"):0;
                readFile(sock, path);
            }
            free(path);
        }
    }
    //pozegnanie
    if(!bye) send(sock, "BYE\n", strlen("BYE\n"), 0);
    verbose?printf("TCP C: closing connection\n"):0;
    close(sock);

}
