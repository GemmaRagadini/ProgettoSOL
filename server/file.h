#ifndef DS_H
#define DS_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "configura_server.h"

/*
- i controlli sullo spazio totale del server devono comprendere il +1?
- lo spazio allocato non conta come memoria occupata? 
-dopo aver fatto elimina un file non funziona cerca_f 
- non è troppo grande la capacità totale del server nel test2? 

*/ 



typedef struct _client{
    int fdc;
    struct _client * next;
}client;

typedef client * lista_client; 

typedef struct _file {
    char pathname[100];
    lista_client * clients; 
    char * contenuto;
    long size_allocata;
    long size_contenuto;
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


int cerca_c(lista_client * C, int fd){
    if (*C == NULL) return -1;
    
    lista_client corrente = *C;
    while (corrente != NULL) {
        if ( (corrente->fdc) ==  fd  )  return 1;
        corrente = corrente->next; 
    }
    return -1;
}


int push_c(lista_client * C, int fd) {

    if (cerca_c(C, fd) == 1) {
        perror("il client ha già aperto questo file, server\n");
        return -1;
    }
    lista_client nuovo = (lista_client)malloc(sizeof(client));
    nuovo->fdc = fd;
    nuovo->next = *C;

    *C = nuovo;
  
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
        if (strcmp(corrente->pathname, pathname) == 0)  return cerca_c( corrente->clients ,fd);

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
            push_c( corrente->clients, fd);
            return 1;
        }
        corrente = corrente->next;
    }

    perror("open_c il file non è presente sullo storage");
    return -1;
}


int cerca_f(Coda_File ds,const char * pathname) { //1 se il file è presente, -1 altrimenti
    if (pathname == NULL) {
        perror("cerca_f : argomento illegale");
        return -1;
    }

    if (ds == NULL) return -1;

    Coda_File corrente = ds;
    while (corrente != NULL) {
        if ( strcmp(corrente->pathname, pathname) == 0 ) return 1;
        corrente = corrente->next; 
    }
    return -1;
}





int eliminaUnFile(Coda_File *ds, Parametri_server * parametri){ 
    printf("Elimino un file\n");
    if (*ds == NULL) {
        perror("impossibile eliminare un file perché lo storage è vuoto");
        return -1;
    }
    Coda_File corrente = *ds;
    while(corrente->next != NULL) corrente = corrente->next;
    parametri->spazio_occupato = parametri->spazio_occupato - corrente->size_contenuto; 

    lista_client cor = *(corrente->clients);
    lista_client prec;
    while (cor != NULL) {
        prec = cor;
        cor = cor->next;
        free(prec);
    }

    free(corrente->clients);

    free(corrente->contenuto);
    free(corrente);
    (parametri->n_file)--;
    return 1;
}


/*ritorna il contenuto del file pathname sullo storage. Se il file è vuoto ritorna NULL*/
char * contenuto_f(Coda_File ds, const char * pathname) {
    if (cerca_f(ds, pathname ) == -1) {
        perror("il file di cui si cerca il contenuto non è presente nel server\n");
        return NULL;
    }
    Coda_File corrente = ds;
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) return corrente->contenuto; //se è null ritorna null
        corrente = corrente->next;
    }
    return NULL;
}


size_t dimensione_f(Coda_File ds, const char* pathname) {
    if (cerca_f(ds, pathname ) == -1) {
        perror("il file di cui si cerca la dimensione non è presente nel server");
        return -1;
    }
    Coda_File corrente = ds;
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) return corrente->size_contenuto;
        corrente = corrente->next;
    }
    return -1;
}



int liberaSpazio(Coda_File *ds, const char * buf, Parametri_server * parametri){
    if (strlen(buf) > parametri->spazio_server) {
        perror("liberaSpazio: lo storage non è abbastanza grande da contenere il file");
        return -1;
    }
    while ( strlen(buf) + parametri->spazio_occupato > parametri->spazio_server) eliminaUnFile(ds, parametri); 
    return 1;
}

    



int append(Coda_File * ds, const char * pathname, char * buf, Parametri_server *parametri, size_t size) { 

    if (*ds == NULL) {
        perror("append: lo storage è vuoto");
        return -1;
    }
    if (cerca_f(*ds, pathname) == -1) {
        fprintf(stderr, "il file %s non è presente nello storage\n", pathname);
        return -1;
    }

    if (size +1  + parametri->spazio_occupato > parametri->spazio_server ) {
        if (liberaSpazio(ds, buf, parametri) == -1) return -1;
    }


    Coda_File corrente = *ds;
    while (corrente != NULL) {

        if (strcmp(corrente->pathname, pathname) == 0) {
            if ( corrente->contenuto == NULL) {

                corrente->size_contenuto = (size+1) * sizeof(char);
                corrente->size_allocata = (size+1) * sizeof(char);

                corrente->contenuto= (char*)malloc(sizeof(char) * (size +1));
                strcpy(corrente->contenuto, buf);

                parametri->spazio_occupato = parametri->spazio_occupato + size +1 ;
                return 1;
            }
            
            while ( corrente->size_allocata < corrente->size_contenuto + size + 1  ) {
                corrente->contenuto = (char*)realloc(corrente->contenuto, 2 * corrente->size_allocata);
                corrente->size_allocata *=2;
            }

            strcat(corrente->contenuto, buf);
            corrente->size_contenuto = corrente->size_contenuto + size + 1;

            parametri->spazio_occupato = parametri->spazio_occupato + size +1;
            return 1;
        }
        corrente = corrente->next;
    }
    
    fprintf(stderr, "append di %s non andata a buon fine\n", pathname);
    return -1;
}
        


int write_f(Coda_File *ds, const char * pathname, char * buffer, Parametri_server * parametri, size_t size) { //il controllo che il client lo abbia aperto viene fatto nel server prima di chiamarla 
    
    if (*ds == NULL ) return -1; 

    if (pathname == NULL) {
        perror ("write_f : pathname illegale");
        return -1;
    }

    if (size + parametri->spazio_occupato > parametri->spazio_server ) {
        if (liberaSpazio(ds, buffer, parametri) == -1) return -1;
    }

    Coda_File corrente = *ds; 
    while (corrente != NULL) {
        if (strcmp(corrente->pathname, pathname) == 0) {
            if (corrente->contenuto == NULL) {
                corrente->size_allocata = (size +1) * sizeof(char);
                corrente->size_contenuto = (size +1) * sizeof(char);
                corrente->contenuto = buffer;
                

                parametri->spazio_occupato = parametri->spazio_occupato + corrente->size_contenuto;
                return 1;
            }
            corrente->contenuto = buffer; //ci riscrive la stessa cosa, non cambia niente a livello di spazio e contenuto 
            return 1;
        }
        corrente = corrente->next;
    }
    return -1;
}







int push_f( Coda_File * ds, char * pathname, int fd, Parametri_server * parametri ) { //quando viene aperto con O_CREATE 
    if (parametri->n_file == parametri->max_file) eliminaUnFile(ds, parametri);

    if (pathname == NULL) {
        perror("push_f: argomento illegale");
        return -1;
    }

    if ( cerca_f(*ds, pathname) == 1 ){
        perror("push di un file già presente nello storage, server\n");
        return -1;
    }

    Coda_File nuovo = (Coda_File)malloc(sizeof(File));
    strcpy(nuovo->pathname, pathname); 

    nuovo->size_allocata = 0;
    nuovo->size_contenuto = 0;
    nuovo->contenuto = NULL;
    nuovo->next = NULL;

    lista_client * L = (lista_client*)calloc(1, sizeof(lista_client));

    nuovo->clients = L; 

    push_c(L, fd);


    if (*ds == NULL) {
        *ds = nuovo;
        (parametri->n_file)++;
        return 1;
    }

    nuovo->next = *ds;
    *ds = nuovo;
    (parametri->n_file)++;

    return 1;
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
            if (cerca_c(corrente->clients,fd) == -1) {
                perror("close di un file che non è stato aperto, server");
                return -1;
            }
            if (remove_c(corrente->clients,fd) != -1) return 1;

        }
        corrente = corrente->next; 
    }
    return -1;
}

void closeStorage(Coda_File *ds) {
    Coda_File corrente = *ds;
    Coda_File precedente;
    while (corrente != NULL){
        precedente = corrente;
        corrente = corrente->next;
        lista_client cor = *(precedente->clients);
        lista_client prec;
        while (cor != NULL) {
            prec = cor;
            cor = cor->next;
            free(prec);
        }

        free(precedente->clients);
        free(precedente->contenuto);
        free(precedente);
    }
}


#endif