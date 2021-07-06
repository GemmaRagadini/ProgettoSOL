#ifndef CONFIGURA_H
#define CONFIGURA_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "tok_c.h"

#define MAX_LEN (BUFSIZ+1)


/*faccio un'unica funzione sia per il client che per il server
- scorro tutto il file per trovare le righe in cui sono indicate le 
    info che mi interessano 
*/

char * nome_socket_client;
const char* dirname;


void errorNUll(char * b, FILE * config){
    if (b == NULL) { 
        perror("malloc buffer");
        fclose(config);
        exit(EXIT_FAILURE);
    }
}

void Configura(FILE * config) {

    char * buffer = NULL; //per tenere la riga letta dal file
    char * ultimo; //per tenere l'ultimo token != NULL
    nome_socket_client = (char*)malloc(20 * sizeof(char));
    
    
    errorNUll(buffer = malloc( MAX_LEN * sizeof(char) ), config);

    while( fgets(buffer, MAX_LEN , config) != NULL ) {
       
        //nome_socket
        if( strstr(buffer, "nome_socket") != NULL) {
            
            errorNUll(ultimo = malloc( MAX_LEN * sizeof(char) ), config);

            ultimo = tokenizer_r(buffer);
            ultimo[strlen(ultimo)-1] = '\0';
            strcpy(nome_socket_client, ultimo);
        }

        
        //dirname
        if( strstr(buffer, "dirname") != NULL) {
           
            errorNUll(ultimo = malloc( MAX_LEN * sizeof(char) ), config);

            ultimo = tokenizer_r(buffer);
            dirname = ultimo;
        }
    }
}

#endif