#ifndef DS_H
#define DS_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "configura_server.h"

/* -se due file hanno lo stesso nome che succede?
- controlla che il rimpiazzamento sia FIFO
- forse quando elimino un file elimina solo il contenuto ma non il pathname->perché ? 
*/ 



typedef struct _client{
    int fdc;
    struct _client * next;
}client;

typedef client * lista_client; 

typedef struct _file {
    char * pathname;
    lista_client clients; 
    char * contenuto;
    struct _file * next;
}File;

typedef File * Coda_File;

void stampaFile(File file){
    printf("nome : %s\n\n %s\n\n", file.pathname, file.contenuto);
}

void stampaLista(Coda_File ds) {
    if (ds == NULL) printf("La lista è vuota\n");
    Coda_File corrente = ds; 
    while (corrente != NULL) {
        stampaFile(*corrente);
        corrente  = corrente->next; 
    }
}

/*calcola lo spazio occupato da un elemento della lista*/
size_t spazioOccupato(File file) { //non sto contando lo spazio occupato dalla lista dei client-> verrebbe molto complicato 
    if (file.contenuto == NULL) return 0;
    return strlen(file.contenuto); 
}


int cerca_c(lista_client C, int fd){
    if (C == NULL) return -1;
    
    lista_client corrente = C;
    while (corrente != NULL) {
        if ( (corrente->fdc) ==  fd  ) return 1;
        corrente = corrente->next; 
    }
    return -1;
}


int push_c(lista_client * C, int fd) {

    if (cerca_c(*C, fd) == 1) {
        perror("il client ha già aperto questo file, server\n");
        return -1;
    }
    lista_client nuovo = (lista_client)malloc(sizeof(client));
    nuovo->fdc = fd;
    nuovo->next = NULL;

    if (*C == NULL) {
        *C = nuovo;
        return 1;
    }

    (*C)->next = nuovo;
    return 1;

}

int remove_c (lista_client *C, int fd) {
    if (*C == NULL) {
        perror("nessun client ha aperto questo file\n");
        return -1;
    }
    lista_client corrente = *C;
    lista_client precedente = corrente;
    if (corrente-> fdc == fd) {
        *C = corrente->next;
        free (corrente);
        return 1;
    }

    while (corrente->next != NULL){
        if (corrente->fdc == fd) {
            precedente->next = corrente->next;
            free(corrente);
            return 1;
        }
        precedente = corrente;
        corrente = corrente->next;
    }

    return -1;
}

int isOpened(Coda_File ds, const char * pathname, int fd) { //1 se pathname è aperto dal client fd 
    if (ds == NULL) {
        perror("lo storage è vuoto, server\n");
        return -1;
    }

    if (pathname == NULL) {
        perror("isOpened: argomento pathname illegale, server");
        return -1;
    }

    Coda_File corrente = ds; 
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) 
            return cerca_c(corrente->clients,fd);
        corrente = corrente->next;
    }

    return -1;
}        


int open_c(Coda_File *ds, const char * pathname, int fd) { //usato quando un client vuole aprire un file già presente sullo storage

    if (pathname == NULL) {
       perror("open_c: argomento pathname illegale, server");
        return -1;
    }

    if (*ds == NULL) {
       perror("open_c :lo storage è vuoto, server");
        return -1;
    } 
    if (isOpened(*ds, pathname, fd ) == 1) {
        perror("open_c: il client ha già aperto questo file");
        return -1;
    }

    Coda_File corrente = *ds;
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0 ) {
            push_c(&(corrente->clients), fd);
            return 1;
        }
        corrente = corrente->next;
    }

    perror("open_c il file non è presente sullo storage");
    return -1;
}


int cerca_f(Coda_File ds,const char * pathname) { //1 se il file è presente, -1 altrimenti 
    if (ds == NULL) return -1;
    Coda_File corrente = ds;
    while (corrente != NULL) {
        if ( strcmp(corrente->pathname, pathname) == 0 ) return 1;
        corrente = corrente->next; 
    }
    return -1;
}





int eliminaUnFile(Coda_File *ds, Parametri_server * parametri){ //elimino un file dal fondo 
    printf("Elimino un file\n");
    if (*ds == NULL) {
        perror("impossibile eliminare un file perché lo storage è vuoto");
        return -1;
    }
    Coda_File corrente = *ds;
    while(corrente->next != NULL) corrente = corrente->next;
    parametri->spazio_occupato = parametri->spazio_occupato- spazioOccupato(*corrente); //va bene? 
    free(corrente);
    (parametri->n_file)--;
    return 1;
}



char * contenuto_f(Coda_File ds, const char * pathname) {
    if (cerca_f(ds, pathname ) == -1) {
        perror("il file di cui si cerca il contenuto non è presente nel server, server\n");
        return NULL;
    }
    Coda_File corrente = ds;
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) {
            if (corrente->contenuto == NULL) {
                perror("il file è vuoto");
                return NULL;
            }
            return corrente->contenuto;
        }
        corrente = corrente->next;
    }
    return NULL;
}



int liberaSpazio(Coda_File *ds, char * buf, Parametri_server * parametri){
    if (strlen(buf) > parametri->spazio_server) {
        perror("liberaSpazio: lo storage non è abbastanza grande da contenere il file");
        return -1;
    }
    while ( strlen(buf) + parametri->spazio_occupato > parametri->spazio_server) eliminaUnFile(ds, parametri); 
    return 1;
}

    



int append(Coda_File * ds, const char * pathname, char * buf, Parametri_server *parametri) { 
    if (*ds == NULL) {
        perror("append: lo storage è vuoto");
        return -1;
    }
    
    if (strlen(buf) + parametri->spazio_occupato > parametri->spazio_server ) {
        if (liberaSpazio(ds, buf, parametri) == -1) return -1;
    }

    Coda_File corrente = *ds;
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) {

            corrente->contenuto=(char*)malloc(strlen(buf)+1);
            if (corrente->contenuto == NULL) {
                strcpy(corrente->contenuto, buf);
                parametri->spazio_occupato = parametri->spazio_occupato + spazioOccupato(*corrente);
                return 1;
            }
            strcpy(corrente->contenuto, strcat(corrente->contenuto, buf));
            parametri->spazio_occupato = parametri->spazio_occupato + spazioOccupato(*corrente);
            return 1;
        }
        corrente = corrente->next;
    }
    parametri->spazio_occupato = parametri->spazio_occupato + spazioOccupato(*corrente);
    return 1;
}
        



int push_f( Coda_File * ds, const char * pathname, int fd, Parametri_server * parametri ) { //quando viene aperto con O_CREATE 

    if (parametri->n_file == parametri->max_file) eliminaUnFile(ds, parametri);

    if ( cerca_f(*ds, pathname) == 1 ){
        perror("push di un file già presente nello storage, server\n");
        return -1;
    }

    Coda_File nuovo = (Coda_File)malloc(sizeof(File));
    nuovo->pathname = (char*)malloc(100 * sizeof(char)); //troppo spazio ? 

    strcpy(nuovo->pathname, pathname);

    nuovo->next = NULL;

    lista_client L = (lista_client)malloc(sizeof(client)); 
    L = NULL;

    push_c(&L, fd);

    nuovo->clients = L; 

    if (*ds == NULL) {
        *ds = nuovo;
        (parametri->n_file)++;
        return 1;
    }

    nuovo->next = *ds;
    *ds = nuovo;
    (parametri->n_file)++;
    printf("%ld\n", parametri->spazio_occupato);
    return 1;
}






int write_f(Coda_File *ds, const char * pathname, char * buffer, Parametri_server * parametri) { //il controllo che il client lo abbia aperto viene fatto nel server prima di chiamarla 
    if (*ds == NULL ) return -1; 
    Coda_File corrente = *ds; 
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) {

            corrente->contenuto = (char*)malloc(strlen(buffer)+ 1);
            strcpy(corrente->contenuto, buffer);
            parametri->spazio_occupato = parametri->spazio_occupato + spazioOccupato(*corrente);
            return 1;
        }
        corrente = corrente->next;
    }
    parametri->spazio_occupato = parametri->spazio_occupato + spazioOccupato(*corrente);
    return -1;
}


int conta_f(Coda_File ds) {//restituisce il numero di file presenti e non vuoti (questo caso forse non si verifica mai )
    int numero = 0;
    if (ds == NULL) return numero;
    Coda_File corrente = ds; 
    while(corrente != NULL) {
        if (contenuto_f(ds, corrente->pathname) != NULL) numero++;
        corrente = corrente->next;
    }
    return numero;
}

    

int close_f(Coda_File *ds, const char * pathname, int fd) { //ritorna 0 in caso di successo, -1 altrimenti

    if (*ds == NULL) return -1;
    if (cerca_f(*ds, pathname) == -1) {
        perror("close di un file che non è presente, server");
        return -1;
    }

    Coda_File corrente = *ds;
    while (corrente != NULL) {
        if ( strcmp(corrente->pathname, pathname) == 0 ) {
            lista_client c = corrente->clients; //puntatore? 
            if (cerca_c(c,fd) == -1) {
                perror("close di un file che non è stato aperto, server");
                return -1;
            }
            if (remove_c(&(corrente->clients),fd) != -1) return 1;

        }
        corrente = corrente->next; 
    }
    return -1;
}









#endif