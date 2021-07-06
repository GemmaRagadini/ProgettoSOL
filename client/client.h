#ifndef LISTATOKEN_H
#define LISTATOKEN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*struttura dati per mantenere i token che (options->uOptions[ (options->uOptionsNumber) -1 ])engono fuori dalla tokenizzazione 
delle stringhe di input del client
*/


typedef struct {
    char ** names;
    int namesNumber;
}Opt;

typedef struct {

    int help; 

    int print; //va messo in tutti i casi 

    time_t waitTime;

    char * socketname; 

    int nFileDaLeggere; //caso R
    char * dirname_Rr; //nome directory per il caso R e r 


    int nFileDaScrivere; //caso w 
    char * dirname_w; //nome directory per il caso w

    Opt WOptions[10]; //per il caso W
    int WOptionsNumber;

    Opt rOptions[10]; //per il caso r 
    int rOptionsNumber;

    Opt lOptions[10]; // caso l 
    int lOptionsNumber;

    Opt uOptions[10]; //caso u 
    int uOptionsNumber;


} Options; 




void tokenizzaEInserisci(char *stringa, Options * options, char k) {
    if (k == 'W') {
        
        (options->WOptionsNumber)++;

        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);
        
        (options->WOptions[(options->WOptionsNumber)-1]).names = (char**)malloc(sizeof(char*) * 20);
        (options->WOptions[(options->WOptionsNumber)-1]).namesNumber = 0;
        (options->WOptions[(options->WOptionsNumber)-1]).names[(options->WOptions[(options->WOptionsNumber)-1]).namesNumber++] = token;
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) (options->WOptions[(options->WOptionsNumber)-1]).names[(options->WOptions[(options->WOptionsNumber)-1]).namesNumber++] = token;
        }
    }
    if (k == 'r') {

        (options->rOptionsNumber)++;

        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);

        (options->rOptions[(options->rOptionsNumber) -1 ]).names = (char **) malloc(sizeof(char*) * 20);
        (options->rOptions[(options->rOptionsNumber) -1 ]).namesNumber = 0; 
        (options->rOptions[(options->rOptionsNumber) -1 ]).names[(options->rOptions[ (options->rOptionsNumber) -1]).namesNumber++] = token;
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) (options->rOptions[(options->rOptionsNumber) -1 ]).names[(options->rOptions[(options->rOptionsNumber)]).namesNumber++] = token;
        }

    }
    if ( k == 'l') {

        (options->lOptionsNumber)++;

        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);

        (options->lOptions[ (options->lOptionsNumber) -1]).names = (char **) malloc(sizeof(char*) * 20);
        (options->lOptions[ (options->lOptionsNumber) -1]).namesNumber = 0; 
        (options->lOptions[ (options->lOptionsNumber) -1]).names[(options->lOptions[ (options->lOptionsNumber) -1]).namesNumber++] = token;
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) (options->lOptions[ (options->lOptionsNumber) -1]).names[(options->lOptions[ (options->lOptionsNumber) -1]).namesNumber++] = token;
        }

    }
    if ( k == 'u') {

        (options-> uOptionsNumber)++; 

        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);

        (options->uOptions[ (options->uOptionsNumber) -1 ]).names = (char **) malloc(sizeof(char*) * 20);
        (options->uOptions[ (options->uOptionsNumber) -1 ]).namesNumber = 0; 
        (options->uOptions[ (options->uOptionsNumber) -1 ]).names[(options->uOptions[ (options->uOptionsNumber) -1 ]).namesNumber++] = token;
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) (options->uOptions[ (options->uOptionsNumber) -1 ]).names[(options->uOptions[ (options->uOptionsNumber) -1 ]).namesNumber++] = token;
        }
    }
    if (k == 'w') {
        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);
        options->dirname_w = token;
        if ( (token = strtok_r(NULL, ",", &tmpstr) ) != NULL) options->nFileDaScrivere = atoi(token);
    }

}

void StampaOpzioni(){
    printf("-h : stampa lista opzioni accettate\n");
    printf("-f filename : specifica il nome del socket AF_UNIX a cui connettersi\n");
    printf("-w dirname[, n=0] : invia al server i file presenti nella cartella dirname\n");
    printf("-W file1[,file2] : invia al server i file passati come argomento\n");
    printf("-r fie1[,file2] : legge dal server i file passati come argomento\n");
    printf("-R [n=0]: legge n file qualsiasi dal server. Se n = 0 li legge tutti\n");
    printf("-d dirname: cartella dove scrivere i file passati con '-r' o '-R'. Deve essere usata congiuntamente a quelle due opzioni\n");
    printf("t time: specifica il tempo in millisecondi che deve intercorrere tra l'invio di due richieste sucessive al server\n");
    printf("-p : abilita le stampe sullo standard output per ogni operazione\n");
}


#endif