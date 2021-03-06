#ifndef CONFIGURA_H
#define CONFIGURA_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "tok_s.h"

/*lettura file di configurazione e configurazione parametri del server*/


#define MAX_LEN (BUFSIZ+1)

typedef struct _parametri_server{

    int num_workers;
    size_t spazio_server; 
    size_t spazio_occupato;
    char* nome_socket_server;
    int max_file;
    int n_file;
    int isClosing;
    int closed;

}Parametri_server;



void errorNUll(char * b, FILE * config){
    if (b == NULL) { 
        perror("malloc buffer");
        fclose(config);
        exit(EXIT_FAILURE);
    }
}

void Configura(FILE * config, Parametri_server * parametri) {


    char * buffer = NULL; //per tenere la riga letta dal file
    char * ultimo; //per tenere l'ultimo token != NULL
    
    errorNUll(buffer = malloc( MAX_LEN * sizeof(char) ), config);

    while( fgets(buffer, MAX_LEN , config) != NULL ) {
        
        //nome_socket
        if( strstr(buffer, "nome_socket") != NULL) {
            ultimo = tokenizer_r(buffer);
            if (ultimo[strlen(ultimo)-1] == '\n')   ultimo[strlen(ultimo)-1] = '\0';
            parametri->nome_socket_server = (char*)malloc( sizeof(char)* (strlen(ultimo)+1));
            strcpy(parametri->nome_socket_server, ultimo);
        }

        //parametri->num_workers
        if( strstr(buffer, "num_workers") != NULL) { 
            ultimo = tokenizer_r(buffer);
            if (ultimo[strlen(ultimo)-1] == '\n')   ultimo[strlen(ultimo)-1] = '\0';
            parametri->num_workers = atoi(ultimo); 
        }

        // parametri->spazio_server 
        if( strstr(buffer, "spazio_server") != NULL) {
            ultimo = tokenizer_r(buffer);
            if (ultimo[strlen(ultimo)-1] == '\n')   ultimo[strlen(ultimo)-1] = '\0';
            parametri->spazio_server = atoi(ultimo);
        }

        //parametri->max_file
        if( strstr(buffer, "max_file") != NULL) {
            ultimo = tokenizer_r(buffer);
            if (ultimo[strlen(ultimo)-1] == '\n')   ultimo[strlen(ultimo)-1] = '\0';
            parametri->max_file = atoi(ultimo);
        }

    }
    free(buffer);
}

#endif