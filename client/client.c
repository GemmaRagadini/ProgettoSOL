#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h> 
#include <sys/syscall.h>
#include <fcntl.h>
#include "API.h"
#include"client.h"
#include "configura_client.h"
#include "utils.h"




/*NOTE : 
- devo considerare che alcune opzioni vanno chiamate in ordine 
- errno ?
- l'ordine delle opzioni influisce  
- optarg nel caso -R 
*/


int main(int argc, char * argv[]) {
    //leggo file di configurazione del client -> tutto ok 
    FILE *config;
    if ( ( config = fopen ("config_client.txt", "r") ) == NULL ) { 
        perror("server : aprendo config_client.txt\n");
        exit(EXIT_FAILURE);
    }
    Configura(config);

    fclose(config); 

    //tempo massimo per openConnection
    struct timespec abstime;
    abstime.tv_sec = 10;

    //struct che contiene le opzioni passate da riga di comando del client
    Options options;

    //campi da inizializzare 
    options.WOptionsNumber = 0;
    options.rOptionsNumber = 0;
    options.uOptionsNumber = 0;
    options.lOptionsNumber = 0;
    options.waitTime = 0;
    options.nFileDaLeggere = 0;
    options.nFileDaScrivere = 0;
    options.help = 0; 
    options.print = 0;
    options.socketname = nome_socket_client;
    options.dirname_w = (char*)malloc( 100 * sizeof(char));
    options.dirname_w = NULL;
    options.dirname_Rr = (char*)malloc( 100 * sizeof(char));
    options.dirname_Rr = NULL;

    int c;

    while ( (c =  getopt(argc, argv, "w:W:r:R::d:t:l:u:c:f:ph") ) != -1 ) {
        switch (c) {
            case 't': 
                options.waitTime = atoi(optarg);
                break;
            case 'p': 
                options.print = 1; //va considerato in tutte le funzioni 
                break;
            case 'h': 
                options.help = 1;
                break;
            case 'f': 
                options.socketname = optarg;
                break;
            case 'd': 
                options.dirname_Rr = optarg;
                break;
            case 'W':
                tokenizzaEInserisci(optarg, &options, 'W');
                break;
            case 'w':
                tokenizzaEInserisci(optarg, &options, 'w');
                break;
            case 'r': 
                tokenizzaEInserisci(optarg, &options, 'r');
                break; 
            case 'l': 
                tokenizzaEInserisci(optarg, &options, 'l');
                break;
            case 'u': 
                tokenizzaEInserisci(optarg, &options, 'u');
                break;
            case 'R': 
                // printf("%s\n", optarg);
                if (optarg != NULL) options.nFileDaLeggere = atoi(optarg);
                else options.nFileDaLeggere = -1; //sarà il caso in cui li leggo tutti 
                break;
        }
    }

    //adesso guardo un campo alla volta e faccio le cose 
    if (options.help == 1) {
        StampaOpzioni();
        exit(EXIT_SUCCESS);
    } 
    if ( strcmp(options.socketname, nome_socket_client ) != 0 ) {
        strcpy(nome_socket_client, options.socketname);
        sleep(options.waitTime);
    }

    //connessione con il server 
    if (openConnection(nome_socket_client, 3, abstime) == -1)  exit(EXIT_FAILURE);
    
    if (options.print==1) printf("client connected\n");



    if (options.WOptionsNumber != 0) {


        for (int i = 0; i < options.WOptionsNumber; i++) {
            for (int j = 0 ; j < (options.WOptions[i]).namesNumber; j++) {

                if (openFile( (options.WOptions[i]).names[j] , O_CREATE) == -1) {

                    if (openFile( (options.WOptions[i]).names[j] , 0 ) == -1) { 
                        if (options.print == 1) fprintf(stderr, "openFile di %s fallita\n", options.WOptions[i].names[j]);
                        exit(EXIT_FAILURE);
                    }

        
                }
                if (options.print==1) printf("openFile di %s effettuata con successo\n", options.WOptions[i].names[j] );

                //qui è stato aperto con successo con O_CREATE




                FILE* file;
                if ( ( file = fopen(options.WOptions[i].names[j], "r") ) == NULL) {
                    if (options.print == 1) fprintf(stderr, "fopen di %s fallita", options.WOptions[i].names[j]);
                    return -1;
                }


                //dimensione del file 
                fseek(file, 0, SEEK_END);
                size_t dim = ftell(file);
                fseek(file, 0, SEEK_SET);

                char * buffer = (char*)malloc( dim * sizeof(char));
                
                if (fread(buffer,sizeof(char), dim, file) != dim ) { 
                    if (options.print == 1) fprintf(stderr, "fread di %s fallita\n", options.WOptions[i].names[j]);
                    return -1;
                }

                size_t size = strlen(buffer);

                if (fclose(file) != 0) {
                    if (options.print == 1) fprintf(stderr, "fclose di %s fallita\n", options.WOptions[i].names[j]);
                    //non lo faccio fallire?
                }


                if (appendToFile(options.WOptions[i].names[j], buffer, size, NULL) == -1){
                    if (options.print == 1) fprintf(stderr, "appendToFile di %s fallita\n", options.WOptions[i].names[j]);
                    exit(EXIT_FAILURE);
                }
                
                if (options.print==1) printf("appendToFile di %s eseguita con successo\n", options.WOptions[i].names[j]);

                if (closeFile(options.WOptions[i].names[j]) == -1) {
                    if (options.print == 1) fprintf(stderr, "closeFile di %s fallita\n", options.WOptions[i].names[j]);
                    exit(EXIT_FAILURE);
                }

                if (options.print==1) printf("closeFile di %s eseguita con successo\n", options.WOptions[i].names[j]);
                
            }
        }
        sleep(options.waitTime);
    }

    if (options.rOptionsNumber != 0) { //aggiungere di salvare in dirname 
        void* buf = (void*)malloc(BUFSIZ * sizeof(void));
        size_t size;
        for (int j = 0; j < options.rOptionsNumber; j++) {
            for (int i = 0 ; i < options.rOptions->namesNumber; i++) {

                if (openFile( (options.rOptions[j]).names[i] , 0 ) == -1) {
                    if (options.print== 1) fprintf(stderr, "openFile di %s fallita\n", options.rOptions[j].names[i]);
                    exit(EXIT_FAILURE);
                }
                if (options.print==1) printf("openFile di %s effettuata con successo\n", options.rOptions[j].names[i] );
            

                if (readFile(options.rOptions[j].names[i], &buf, &size) == -1) {
                    if (options.print == 1) fprintf(stderr,"readFile di %s fallita\n", options.rOptions[j].names[i]);
                    exit(EXIT_FAILURE); //un po' too much? 
                }


                //lo salvo nella cartella dirname se è specificata
                if (options.dirname_Rr != NULL) {
                    
                     //estraggo il nome della stringa dal pathname
                    char * tmpstr;
                    char * nomefile = (char*)malloc( sizeof(char) * (strlen(options.rOptions[j].names[i]) +1));
                    strcpy(nomefile, options.rOptions[j].names[i]);
                    char * str = strtok_r(nomefile, "/", &tmpstr); 
                    while (str != NULL){
                        nomefile= str; 
                        str = strtok_r(NULL, "/", &tmpstr );
                    }

                    char * nomecompleto = (char*)malloc((strlen(options.dirname_Rr)+ strlen(nomefile)+ 1)*sizeof(char));
                    strcpy( nomecompleto,dirname);
                    nomecompleto[strlen(nomecompleto)-1] = '\0';
                    strcat(nomecompleto,"/");
                    strcat(nomecompleto,nomefile);

                    FILE * fd = fopen(nomecompleto , "w");

                    if (fputs(buf,fd) == -1) {
                        if (options.print == 1 ) fprintf(stderr, "readFile di %s fallita\n", nomecompleto );
                        return -1;
                    }

                    if (fclose(fd) == -1) {
                        fprintf(stderr, "readNFiles: fclose di %s fallita\n", nomecompleto);
                        return -1;
                    }

                    free(nomecompleto);//probabilmente ci vogliono anche altre free
                }

                
                if (options.print==1) printf("readFile di %s effettuata con successo\n", options.rOptions[j].names[i] );

                if (closeFile(options.rOptions[j].names[i]) == -1) {
                    if (options.print == 1) fprintf(stderr,"closeFile di %s fallita\n", options.rOptions[j].names[i]);
                    exit(EXIT_FAILURE);
                }

                if (options.print==1) printf("closeFile di %s effettuata con successo\n\n", options.rOptions[j].names[i] );

            }

        }
        sleep(options.waitTime);
    }



    if (options.dirname_w != NULL) { //caso w 
        w_file_selector(options.dirname_w, &(options.nFileDaScrivere), options.print); //chiama la append sui i file nella directory dirname 
        sleep(options.waitTime);
    }



    if (options.nFileDaLeggere != 0) {
        if (options.dirname_Rr == NULL) {
            if (options.print == 1) fprintf(stderr, "L'opzione R può essere usata solo congiuntamente all'opzione d\n");
            exit(EXIT_FAILURE);
        }
        if (readNFiles(options.nFileDaLeggere, dirname) == -1) {
            fprintf(stderr, "readNFiles fallita\n");
            exit(EXIT_FAILURE);
        }
        if (options.print==1) printf("readNFiles effettuata con successo\n");
        sleep(options.waitTime);
    }


    if (closeConnection(nome_socket_client) == -1){
        if (options.print == 1) fprintf(stderr, "closeConnection fallita\n");
        exit(EXIT_FAILURE);
    }

    if (options.print==1) printf("Client disconnected\n");
    
    return 0;
}    

        
