#ifndef WORKS_H
#define WORKS_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

/* struttura dati per i lavori che i client chiedono di eseguire al server*/


typedef struct _work
{
    char pathname[50];
    char tipo_operazione;
    int fd;
    struct _work *next;
}JobElement;

typedef JobElement * JobList;

void Push_W(JobList * coda, JobList job){
    if (*coda == NULL) {
        *coda = job;
        return;
    }
    job->next = *coda;
    *coda = job;
}


void Stampa_W(JobList coda) {
    if (coda == NULL) {
        printf("La coda di lavori è vuota"); 
        return;
    }
    JobList corrente = coda;
    while (corrente != NULL) {
        printf("%s\n", corrente->pathname);
        corrente = corrente->next;
    }
}

JobList Pop_W(JobList *s) {
    if (*s == NULL) return NULL;  
    JobList estratto;

    if ((*s)->next == NULL) {
        estratto = *s; 
        *s = NULL; 
        return estratto;
    }
    JobList corrente = *s;
    if (corrente->next == NULL ){
        *s = NULL;
        return corrente;
    }
    while (corrente->next->next  != NULL) corrente = corrente->next;
    estratto = corrente->next;
    corrente->next = NULL;
    return estratto;
}

void closeListaJob(JobList *s){
    JobList corr = *s;
    JobList next;
    while (corr != NULL) {
        next = corr->next;
        //free(corr->pathname);
        free(corr);
        corr = next;
    }
}

        

#endif