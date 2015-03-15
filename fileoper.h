#ifndef FILE_OPER_H 
#define FILE_OPER_H 

#include "list.h"

//wysyła plik o sciezce path na socket sock
int sendFile(int sock, const char* path);
//wczytuje dane z pliku filename i wstawia do listy dest
int readShared(List* dest,const char* filename);
//wysyla liste files na socket sock 
int sendList(int sock, List* files);
//wczytuje plik filename z socketa sock
int readFile(int sock, const char* filename);
//bezpieczna wersja linuksowego basename. Alokuje pamiec. 
char* getFilename (const char* path);
//zwraca ścieżkę z nazwy katalogu i pliku. Alokuje pamiec
char* getPath (const char* dir, const char* file);
//dodaje filename 
int addToEnd(const char* filename, const char* toAdd);

#endif
