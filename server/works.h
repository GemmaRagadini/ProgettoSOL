#ifndef WORKS_H
#define WORKS_H

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

//DOVREBBE SERVIRE PER IMPLEMENTARE UNA coda DI LAVORI CHE ARRIVANO AL SERVER E SU CUI I WORKER FARANNO LE POP 
//GIÀ TESTATA E FUNZIONA in prova_coda


typedef struct _work
{
    char pathname[50];
    char tipo_operazione;
    /* 
    - X per ancora da valutare 
    - o per openFile
    - r per readFile
    - R per readNFiles
    - w per writeFile
    - a per appendToFile
    - c per closeFile
    - da aggiungere quelle che mancano 
    */ 

    size_t size;
    void* buf;
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
    if ((*s)->next == NULL) { //se c'è un solo job
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


#endif