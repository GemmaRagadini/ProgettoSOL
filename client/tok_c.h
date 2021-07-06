#ifndef TOK_H
#define TOK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* per tokenizzare le stringhe nel file di configurazione */

char * tokenizer_r(char *stringa) { //restituisce l'ultima parola della riga 
    char *tmpstr;
    char *vecchio = NULL;
    char *token = strtok_r(stringa, " ", &tmpstr);
    while (token) {
        vecchio = token;
        token = strtok_r(NULL, " ", &tmpstr);
    }
    return vecchio;
}



#endif