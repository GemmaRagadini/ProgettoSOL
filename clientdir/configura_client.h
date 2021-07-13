#ifndef CONFIGURA_H
#define CONFIGURA_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "tok_c.h"
#include "client.h"

#define MAX_LEN (BUFSIZ+1)

/*lettura file di configurazione e configurazione parametri del client*/

void errorNUll(char * b, FILE * config){
    if (b == NULL) { 
        perror("malloc buffer");
        fclose(config);
        exit(EXIT_FAILURE);
    }
}

void Configura(FILE * config, Options *  options) {

    char * buffer = NULL; //per tenere la riga letta dal file
    char * ultimo; //per tenere l'ultimo token != NULL
    
    
    errorNUll(buffer = malloc( MAX_LEN * sizeof(char) ), config);

    while( fgets(buffer, MAX_LEN , config) != NULL ) {
       
        //nome_socket
        if( strstr(buffer, "nome_socket") != NULL) {
            
            ultimo = tokenizer_r(buffer);
            if (ultimo[strlen(ultimo)-1] == '\n')   ultimo[strlen(ultimo)-1] = '\0';
            options->nome_socket_client = (char*)malloc(sizeof(char) * ( strlen(ultimo) + 1) );
            strcpy(options->nome_socket_client, ultimo);

        }

    }
    free(buffer);
}

#endif