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



void w_file_selector(char* dirname, int* n, int p) {
    FILE * fl;

    if (chdir(dirname) == -1) {
        if (p == 1) {
            fprintf(stderr, "Errore durante il tentativo di posizionamento nella directory %s\n", dirname);
        } 
        return;
    }

    // Se ho gia' aperto n file esco
    if (n != NULL && n == 0) {
        return;
    }

    DIR * dir = opendir(dirname);
    if (dir == NULL) {
        if (p == 1) {
            fprintf(stderr, "Errore aprendo la directory %s\n", dirname);
        }
        return;
    } 
    else {
        struct dirent* file;
        
        while((errno = 0, file = readdir(dir)) != NULL) {
            // Se ho gia' aperto n file esco
            if (n != NULL && n == 0) {
                return;
            }
            struct stat info;
            if (stat(file->d_name, &info) == -1) {
                if (p == 1) {
                    fprintf(stderr, "Errore durante la stat su %s/%s\n", dirname, file->d_name);
                    perror("");
                }
                continue;
            }
            else {
                if (S_ISDIR(info.st_mode)) {
                    if ((strcmp(file->d_name, ".") != 0) && (strcmp(file->d_name, "..") != 0) && file->d_name[0] != '.') {
                        char new_dirname[FILENAME_MAX];
                        sprintf(new_dirname, "%s/%s", dirname, file->d_name);
                        //printf("dirname: %s/%s\n", dirname, file->d_name); // DEBUG
                        w_file_selector(new_dirname, n, p);
                        if (chdir(dirname) == -1) {
                            if (p == 1) {
                                fprintf(stderr, "Errore durante il tentativo di posizionamento nella directory %s\n", dirname);
                            } 
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


                        char * buffer  = (char *)malloc(dim * sizeof(char));
                        if (fread(buffer, sizeof(char), dim , fl) == -1) {
                          perror("w_file_selector: fread fallita, client"); 
                          return;
                        }      


                        if (openFile(file->d_name, O_CREATE) == -1) {
                          if (openFile(file->d_name, 0)== -1) {
                            perror("w_file_selector: openFile fallita, client"); 
                            return;
                          }
                        }

                        if (p == 1) printf("openFile di %s effettuata con successo\n", file->d_name);

                        if (appendToFile(file->d_name, buffer, dim, NULL) == -1) {
                          perror("w_file_selector: appendToFile fallita, client");
                          return;
                        }

                        if (p == 1) printf("appendToFile di %s eseguita con successo\n", file->d_name);

                        if (n != NULL) {
                            *n -= 1;
                        }
                        if (fclose(fl) != 0) {
                          fprintf(stderr, "Errore durante la chiusura del file %s/%s\n", dirname, file->d_name);
                        return;
                        }

                        if (closeFile(file->d_name) == -1) {
                          perror("closeFile fallita, client");
                          exit(EXIT_FAILURE);
                        }

                        free(buffer);
                        if (p == 1) printf("closeFile di %s eseguita con successo\n", file->d_name);
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