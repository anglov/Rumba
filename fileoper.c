#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libgen.h>

#include "fileoper.h"

#define BUFFSIZE 2048

int sendFile(int sock, const char* path){
    int file = open(path, O_RDONLY);
    if (file<0) {
        perror("Error while open file");
        return file;
    }
    char buff[BUFFSIZE];
    while(1){
        int len = read(file, buff, sizeof(buff));
        if(len < 0) {perror("Error while reading file"); return len;}
        else if(len == 0) return 0;
        send(sock, buff, len,0);
    }
}

int readFile(int sock, const char* filename){
    int file = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (file<0) {
        perror("Error while creating file");
        return file;
    }
    char buff[BUFFSIZE];
    int len;
    while(1){
        len=recv(sock,buff,sizeof(buff),0);
        if(len < 0) {perror("Error while reading file"); close(file); return len;}
        else if(len == 0) {close(file); return 0;}
        write(file, buff, len);
    }
}

int readShared(List* dest, const char* filename) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;
    while ((read = getline(&line, &len, fp)) != -1) {
        *strchr(line,'\n')='\0';
        addEnd(dest, newElemString(line));
    }
    if (line)
        free(line);
    fclose(fp);
    return 0;
}

int sendList(int sock, List* files){
    if(isEmpty(files)) return 1;
    int i=1;
    for(ListElem *ptr=files->head;ptr;ptr=ptr->next){
        char *res;
        res = getFilename((char*)ptr->d);
        size_t size = strlen(res)+50; //wiem, to nie jest dobra praktyka
        char str[size];
        snprintf(str,size,"%d. %s\n",i,res);
        send(sock, str, strlen(str),0);
        ++i;
        free(res);
    }
    return 0;    
}

char* getFilename (const char* path) {
    char *tmp, *in;
    in = strdup(path);
    tmp=basename(in);
    size_t size = strlen(tmp);
    char* res = malloc(size*sizeof(char)+1);
    strcpy(res, tmp);
    free(in);
    return res;
}

char* getPath (const char* dir, const char* file) {
    size_t size = strlen(dir) + strlen(file);
    char* res;
    if (dir[strlen(dir)-1] != '/') {
        res = malloc(size*sizeof(char)+2);
        snprintf(res, size+2,"%s/%s",dir,file);
    }
    else {
        res = malloc(size*sizeof(char)+1);
        snprintf(res, size+2,"%s%s",dir,file);
    }
    return res;
}

int addToEnd(const char* filename, const char* toAdd){
    int file = open(filename, O_CREAT|O_WRONLY|O_APPEND, 0666);
    if (file<0) {
        perror("Error while opening file");
        return file;
    }
    if(access(toAdd, R_OK)) {
        perror("File error");
        return 1;
    }
    write(file, toAdd, strlen(toAdd));
    write(file, "\n", strlen("\n"));
    printf("Successful adding '%s' to shared resources\n", toAdd);
    return 0;
}
