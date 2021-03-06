#ifndef LISTATOKEN_H
#define LISTATOKEN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

/*gestione opzioni del client*/

typedef struct {
    char ** names;
    int namesNumber;
}Opt;

typedef struct {

    int help; 

    int print; //va messo in tutti i casi 

    time_t waitTime;

    int nFileDaLeggere; //caso R
    char * dirname_Rr; //nome directory per il caso R e r 


    int nFileDaScrivere; //caso w 
    char * dirname_w; //nome directory per il caso w

    Opt WOptions[10]; //per il caso W
    int WOptionsNumber;

    Opt rOptions[10]; //per il caso r 
    int rOptionsNumber;

    char * nome_socket_client;


} Options; 



void tokenizzaEInserisci(char *stringa, Options * options, char k) {
    if (k == 'W') {
        
        (options->WOptionsNumber)++;

        // controllo che i file passati esistano 
        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);
        if (access(token, F_OK) != 0 ) {
            fprintf(stderr, "Il file %s non è esistente\n", token);
            return;
        }
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) {
                
                if (access(token, F_OK) != 0 ) {
                    fprintf(stderr, "Il file %s non è esistente\n", token);
                    return;
                }
            }
        }

        token = strtok_r(stringa, ",", &tmpstr);
        Opt * temp = &(options->WOptions[(options->WOptionsNumber) -1 ]);

        temp->names = (char**)malloc(sizeof(char*) * 200);
        temp->namesNumber = 0;

        temp->names[temp->namesNumber] = (char*)malloc(sizeof(char) * ( strlen(token) +1));
        strcpy ( temp->names[temp->namesNumber++],token);


        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) {
                
                temp->names[temp->namesNumber++] = (char*)malloc(sizeof(char) * ( strlen(token) +1));
                strcpy ( temp->names[temp->namesNumber],token);
            }
        }
    }
    if (k == 'r') {

        (options->rOptionsNumber)++;

        char *tmpstr;
        char *token = strtok_r(stringa, ",", &tmpstr);
        Opt * temp = &(options->rOptions[(options->rOptionsNumber) -1 ]);
        
        temp->names = (char **) malloc(sizeof(char*) * 200);
        temp->namesNumber = 0; 

        temp->names[temp->namesNumber] = (char*)malloc(sizeof(char) * ( strlen(token) +1));


        strcpy ( temp->names[temp->namesNumber++],token);
        
        while (token) {
            token = strtok_r(NULL, ",", &tmpstr);
            if (token != NULL) {
                
                temp->names[temp->namesNumber++] = (char*)malloc(sizeof(char) * ( strlen(token) +1));
                strcpy ( temp->names[temp->namesNumber++],token);
            }
        }

    }
    
    if (k == 'w'){
        char *tmpstr;
        char * token = strtok_r(stringa, ",", &tmpstr);
        //controllo che la directory sia esistente 
        DIR * dir = opendir(token);
        if (!dir) {
            fprintf(stderr, "La directory %s non è esistente\n", token);
            return;
        }

        options->dirname_w = (char*)malloc( ( strlen(token)+1 ) * sizeof(char));
        strcpy(options->dirname_w, token);
        if ( (token = strtok_r(NULL,  ",", &tmpstr) ) == NULL) options->nFileDaScrivere = -1;
        else {
            options->nFileDaScrivere = atoi(token);
            if (options->nFileDaScrivere == 0) options->nFileDaScrivere = -1;
        }   
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

void closeOptions(Options * options){
    
    free(options->dirname_w);
    free(options->dirname_Rr);
    free(options->nome_socket_client);

    for (int i = 0; i < options->WOptionsNumber; i++) {
        for (int j = 0; j < options->WOptions->namesNumber; j++)    {
            free( (options->WOptions[i]).names[j]);
        }
    }
    for (int i = 0; i < options->rOptionsNumber; i++) {
        for (int j = 0; j < options->rOptions->namesNumber; j++)   { 
        free( (options->rOptions[i]).names[j]);
        }
    }

}


#endif