#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "API.h"

/*funzione per la ricerca e la richiesta di scrittura di tutti i file della directory indicata*/


void w_file_selector(char* dirname, int* n, int p) {
    
    FILE * fl;

    if (chdir(dirname) == -1) {
            fprintf(stderr, "Errore durante il tentativo di posizionamento nella directory %s\n", dirname);
        return;
    }

    // Se ho gia' aperto n file esco
    if (n != NULL && *n == 0) {
        return;
    }

    DIR * dir = opendir(dirname);
    if (dir == NULL) {
        fprintf(stderr, "Errore aprendo la directory %s\n", dirname);

        return;
    } 
    else {
        struct dirent* file;
        
        while((errno = 0, file = readdir(dir)) != NULL) {
            // Se ho gia' aperto n file esco
            if (n != NULL && *n == 0) {
                return;
            }
            struct stat info;
            if (stat(file->d_name, &info) == -1) {
                fprintf(stderr, "Errore durante la stat su %s/%s\n", dirname, file->d_name);
                perror("");
                continue;
            }
            else {
                if (S_ISDIR(info.st_mode)) {
                    if ((strcmp(file->d_name, ".") != 0) && (strcmp(file->d_name, "..") != 0) && file->d_name[0] != '.') {
                        char new_dirname[FILENAME_MAX];
                        sprintf(new_dirname, "%s/%s", dirname, file->d_name);
                        w_file_selector(new_dirname, n, p);
                        if (chdir(dirname) == -1) {
                            fprintf(stderr, "Errore durante il tentativo di posizionamento nella directory %s\n", dirname);
                            return;
                        }
                    }
                }
                else {

                  fl = fopen(file->d_name, "r");
                   if (fl == NULL) {
                        fprintf(stderr, "Errore durante l'apertura del file %s/%s\n", dirname, file->d_name);
                        return;
                    }
                    else {

                      //cerco lunghezza del file 

                        fseek(fl, 0, SEEK_END);
                        size_t dim = ftell(fl);
                        fseek(fl, 0, SEEK_SET);
                        char * nomecompleto = (char*)malloc((strlen(dirname)+ strlen(file->d_name)+ 1)*sizeof(char));
                        strcpy(nomecompleto, dirname);
                        nomecompleto[strlen(nomecompleto)] = '\0';
                        strcat(nomecompleto,"/");
                        strcat(nomecompleto,file->d_name);

                        char * buffer  = (char *)malloc( (dim+1) * sizeof(char));
                        memset(buffer,0,dim+1);

                        if (fread(buffer, sizeof(char), dim , fl) != dim) {
                          perror("w_file_selector: fread fallita, client"); 
                          return;
                        }      


                        if (openFile(nomecompleto, O_CREATE) == -1) {
                          if (openFile(nomecompleto, 0)== -1) {
                            perror("w_file_selector: openFile fallita, client"); 
                            return;
                          }
                        }

                        if (p == 1) printf("openFile di %s effettuata con successo\n", nomecompleto);
                        
                        if (appendToFile(nomecompleto, buffer, dim, NULL) == -1) {
                          perror("w_file_selector: appendToFile fallita, client");
                          return;
                        }

                        if (p == 1) printf("appendToFile di %s eseguita con successo\n", nomecompleto);

                        if (n != NULL) {
                            *n -= 1;
                        }
                        if (fclose(fl) != 0) {
                          fprintf(stderr, "Errore durante la chiusura del file %s/%s\n", dirname, nomecompleto);
                        return;
                        }

                        if (closeFile(nomecompleto) == -1) {
                          perror("closeFile fallita, client");
                          exit(EXIT_FAILURE);
                        }

                        if (p == 1) printf("closeFile di %s eseguita con successo\n", nomecompleto);
                        free(nomecompleto);
                        free(buffer);
                    }
                }
            }
        }

        if (errno != 0) {
            perror("Erore sulla readdir");
        }

        closedir(dir);
    }
}


#endif