#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h> 
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include "works.h"
#include "mutex.h"
#include "tok_s.h"
#include "file.h"
#include "configura_server.h"


int fd_skt; 

JobList * coda; //coda di jobs che devono essere eseguiti



Coda_File * storage; //struttura dati per i file 

Parametri_server parametri; //struttura in cui stanno i parametri presi dal file di configurazione


static pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER; //per la signal sulla coda di work 
static pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER; //mutex per la coda di work 
static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER; //mutex per la DS dei file 


#define O_CREATE 8
#define O_LOCK 4 
#define NOFLAGS 0




int Open( char * pathname,int fd, int flags) {

    if (pathname == NULL) {
        perror("Open: pathname illegale, server\n");
        return -1;
    }


    switch (flags) {
        case O_CREATE|O_LOCK:;
        case O_CREATE: 

            if (cerca_f(*storage, pathname) == 1)   return -1;


            if (push_f(storage, pathname, fd, &parametri) == -1) {
                perror("Open: errore nel caricamento del file, server\n");
                return -1;
            }

            return 0;

        case O_LOCK:;
        case 0: 

            if (cerca_f(*storage, pathname) == -1) {
                printf("fallita apertura di un file non presente nello storage\n");
                return -1;
            }

            if (open_c(storage, pathname, fd) == -1 ) {
                perror("Open: errore nell'apertura del file , server");
                return -1;
            }
         
            return 0;
        default:
            perror("openFile: flag illegale,server\n");
            return -1;
    }
}



int Write(const char * pathname, int fd) {


    if (pathname == NULL) {
        perror("Write : pathname illegale, server");
        return -1;
    }

    if (cerca_f(*storage,pathname) == -1) {
        perror ("Write: il file non è presente nello storage");
        return -1;
    }

    if (isOpened(*storage,pathname,fd) == -1) {
        perror("Write : il file non è stato aperto dal client");
        return -1;
    }

    size_t dim; 
    if (read(fd, &dim, sizeof(size_t)) == -1) {
        perror("Write: read della dimensione del file fallita");
        return -1;
    }
    char *buffer= (char*)calloc(dim, 1);  
    int cnt = 0;
    
    while ( (cnt += read(fd, buffer, dim)) > 0) {
        if (cnt == dim) break;
    } 
   
  
    if (write_f(storage, pathname, buffer, &parametri, dim) == -1) { 
        perror ("Write: scrittura non andata a buon fine , server");
        return -1;
    }

    

    return 0;
}



int Close(const char * pathname, int fd) {

    if (pathname == NULL) {
        perror("Write : pathname illegale, server");
        return -1;
    }

    if (close_f(storage,pathname, fd) == -1) {
        perror("Close: errore in chiusura, server");
        return -1;
    }

    return 0;
}




int Read(const char * pathname, int fd, char ** buf ,size_t *size) {


    if (pathname == NULL) {
        perror("Read : pathname illegale, server");
        return -1;
    }

    if (*storage == NULL) {
        fprintf(stderr, "la lettura di %s è fallita perché lo storage è vuoto\n", pathname);
        return -1;
    }

    if (cerca_f(*storage, pathname) == -1) {
        perror("Read: il file non è presente nello storage, server");
        return -1;
    }

    if (isOpened(*storage, pathname, fd) == -1 ) {
        perror("Read: il file non è stato aperto dal client, server");
        return -1;
    }
    
    *buf = contenuto_f(*storage, pathname );
    *size = dimensione_f(*storage, pathname);
    return 1;
}





int Append(char * pathname, int fd, char * buf, size_t size) {

    if (pathname == NULL ) {
        perror("Append : pathname illegale, server");
        return -1;
    }

    if (buf == NULL) {
        perror("Append: buffer per scrittura vuoto, server");
        return -1;
    }

    if (isOpened(*storage, pathname, fd) == -1) { 
        perror("Append: il file non è stato aperto dal client, server");
        return -1;
    }

    if (append(storage, pathname, buf, &parametri, size) == -1) {
        perror("Append: scrittura fallita, storage");
        return -1;
    }

    return 0;
}



int readN(int N, int fd){
    int n = conta_f(*storage); 
    if (n == 0) return n;  
    if (N <= 0) N = n;
    if (n > N) n = N;
   
    int cont = 0; 
    int l_pathname; 
    int l_buffer; 
    char * buffer;
    Coda_File corrente = *storage;
    if (write(fd, &n, sizeof(int)) == -1) {
        perror("readN : SC read numero letti, server");
        return -1;
    }

    while (cont < n && corrente != NULL) {

        if (contenuto_f(*storage, corrente->pathname) != NULL) {  
            buffer = contenuto_f(*storage, corrente->pathname);
            l_pathname = (int)strlen(corrente->pathname);
            l_buffer = (int)strlen(buffer);
          
            if (write(fd, &l_pathname, sizeof(int)) == -1) {
                perror("readN : SC read lunghezza pathname, server");
                return -1; 
            }

            if (write(fd, corrente->pathname, l_pathname +1) == -1) {
                perror("readN : SC read pathname, server");
                return -1;
            }
            if (write(fd, &l_buffer, sizeof(int)) == -1) {
                perror("readN : SC read lunghezza buffer, server");
                return -1;
            }

            if (write(fd, buffer, l_buffer+1) == -1) {
                perror("readN : SC read buffer, server");
                return -1;
            }

            cont++; 
        }

        corrente = corrente->next;
    }

    if (cont != n) return -1;
    return cont;
}
    

        

static void * worker(void * arg){
    
    while(!parametri.closed) {

        Pthread_mutex_lock(&mtx1);

        if ( (*coda == NULL && parametri.isClosing) || parametri.closed) {
            Pthread_mutex_unlock(&mtx1);
            break;
        }
        while (*coda == NULL && !parametri.closed && !parametri.isClosing)  pthread_cond_wait(&cond, &mtx1);
           
        if ( (*coda == NULL && parametri.isClosing) || parametri.closed) {
            Pthread_mutex_unlock(&mtx1);
            break;
        }
        
        JobList estratto;

        estratto = Pop_W(coda);
        Pthread_mutex_unlock(&mtx1);
        
        char tipo_op; //mi serve per ricordarmelo anche dopo aver fatto la free(estratto) 
        int fd; //mi serve per lo stesso motivo 

        int ret; //valore da ritornare al client 

        int l; //grandezza del pathname

        int N; //numero file da leggere nel caso R 
        
        size_t size;

        if (estratto->tipo_operazione == 'X') {
            if (read(estratto->fd, &(estratto->tipo_operazione), sizeof(char)) <= 0){
                continue; 
            }
        }
        if (estratto->tipo_operazione != 'z') printf("%c\n", estratto->tipo_operazione);

        char pathname[100];

        char ** buf = (char**)malloc(sizeof(char*));


        switch(estratto->tipo_operazione) {
            case 'o':

                if (read(estratto->fd, &l, sizeof(int)) == -1) {
                    perror("worker: SC read length, server\n");
                    ret = -1;
                    break;
                }


                if ( read(estratto->fd, pathname, l+1) == -1) {
                    perror("worker : SC read pathname, server\n");
                    ret = -1;
                    break;
                }   

                int flags;
                if (read(estratto->fd, &flags, sizeof(int) ) == -1) {
                    perror("worker : SC read flags, server\n");
                    ret = -1;
                    break;
                }

                Pthread_mutex_lock(&mtx2);
                ret = Open(pathname,estratto->fd, flags);
                Pthread_mutex_unlock(&mtx2);

                break;

            case 'r' : 
                if ( read(estratto->fd, &l, sizeof(int)) == -1) {
                    perror("worker: SC read length, server\n");
                    ret = -1;
                    break;
                }

                if ( read(estratto->fd, pathname, l+1) == -1) {
                    perror("worker : SC read pathname, server\n");
                    ret = -1;
                    break;
                }


                Pthread_mutex_lock(&mtx2);


                ret = 0;

                if ( Read(pathname, estratto->fd, buf, &size )== -1) {
                    fprintf(stderr,"lettura del file %s dallo storage fallita\n", pathname);
                    ret = -1;
                }

                Pthread_mutex_unlock(&mtx2);

                if (write(estratto->fd, &ret, sizeof(int)) <= 0) {
                    perror("worker:SC write ret, server\n");
                    break;
                }

                if (ret == -1) break; 

                if (write(estratto->fd, &size, sizeof(size_t) ) == -1 ) {
                    perror("worker : SC write size, server");
                    break;

                }

                int cont = 0;
                while( (cont+= write(estratto->fd, *buf, size) )> 0) { 
                    if (cont == size) break;
                }

                break;
            case 'R' :
                if (read(estratto->fd, &N, sizeof(int) ) == -1) {
                    perror("worker: SC read numero file da leggere, server\n");
                    ret = -1;
                    break;
                }

                Pthread_mutex_lock(&mtx2); 
                ret = readN(N, estratto->fd);
                Pthread_mutex_unlock(&mtx2);

                break;
            case 'W' : 

                    if ( read(estratto->fd, &l, sizeof(int)) == -1) {
                    perror("worker: SC read length, server\n");
                    ret = -1;
                    break;
                    }

                    if (read(estratto->fd, pathname, l+1 ) == -1) {
                        perror("worker, SC read pathname, server\n");
                        ret = -1;
                        break;
                    }

                    Pthread_mutex_lock(&mtx2);
                    ret = Write(pathname, estratto->fd);
                    Pthread_mutex_unlock(&mtx2);


                break;
            case 'a' :

                    if ( read(estratto->fd, &l, sizeof(int)) == -1) {
                    perror("worker: SC read length, server");
                    ret = -1;
                    break;
                    }

                    if (read(estratto->fd, pathname, l+1) == -1) {
                        perror("worker,SC read pathname, server");
                        ret = -1;
                        break;
                    }

                    if (read(estratto->fd, &size, sizeof(size_t)) == -1) {
                        perror("worker: SC read size, server");
                        ret = -1;
                        break;
                    }
                    if ( (size+1) > parametri.spazio_server )  {
                        ret = -1;
                        close(estratto->fd);
                        break;
                    }
                    
                    *buf= (char*)calloc(size +1, 1);  

                    int cnt = 0;
                    while ( (cnt += read(estratto->fd, *buf, size +1)) > 0) {
                        if (cnt == size+1) break;
                    }                        

                    Pthread_mutex_lock(&mtx2);
                    ret = Append(pathname, estratto->fd, *buf, size);
                    Pthread_mutex_unlock(&mtx2);

                    free(*buf);


                break;

            case 'c' : 

                    if ( read(estratto->fd, &l, sizeof(int)) == -1) {
                    perror("worker: SC read length, server\n");
                    ret = -1;
                    break;
                    }

                    if (read(estratto->fd, pathname, l+1) == -1) {
                        perror("worker,SC read pathname, server\n");
                        ret = -1;
                        break;
                    }
                    Pthread_mutex_lock(&mtx2);
                    ret = Close(pathname, estratto->fd);
                    Pthread_mutex_unlock(&mtx2);

                break;
            default : 
                break;
        }

        if (estratto->tipo_operazione != 'r' && estratto->tipo_operazione != 'z') {
            if (write(estratto->fd, &ret, sizeof(int)) <= 0) {
                perror("client disconnected, cannot send response\n");
            }
        }

        tipo_op = estratto->tipo_operazione;
        fd = estratto->fd;


        if (ret == -1 && tipo_op != 'o') { //se l'operazione fallita la open non è stato aperto nessun file -> altrimenti viene chiuso 
            Pthread_mutex_lock(&mtx2);
            tipo_op = 'c';
            Close(pathname, estratto->fd);
            Pthread_mutex_unlock(&mtx2);
        }

        char k;

        free(buf);
        free(estratto);


        if (read (fd, &k, sizeof(char)) > 0 && !parametri.closed ){ 
            if (k != 'z') {//z -> il client si sta per disconnettere e non manderà più richieste  
                JobList job = (JobList)malloc(sizeof(JobElement)); 
                job->tipo_operazione = k; 
                job->fd = fd;
                job->next = NULL;
            

                Pthread_mutex_lock(&mtx1);
                Push_W(coda, job);
                pthread_cond_signal(&cond);  
                Pthread_mutex_unlock(&mtx1); 
            }
        }
     
    }

    return (void*)0;
}




void sighandler(int sig){
    switch(sig){
        
        case SIGHUP:
            close(fd_skt);
            parametri.isClosing = 1;
            Pthread_mutex_lock(&mtx1);
            pthread_cond_broadcast(&cond);  
            Pthread_mutex_unlock(&mtx1);
         break;
        
        case SIGQUIT:

        case SIGINT:{
            close(fd_skt);
            parametri.closed = 1;
            Pthread_mutex_lock(&mtx1);
            pthread_cond_broadcast(&cond);  
            Pthread_mutex_unlock(&mtx1);
        }break;
     
        default:{
            abort();
        }
    }
}
    

void installSigHand(){

    struct sigaction sa;
    sigset_t handlermask;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighandler;
    
    sigemptyset(&handlermask);
    sigaddset(&handlermask, SIGHUP);
    sigaddset(&handlermask, SIGQUIT);
    sigaddset(&handlermask, SIGINT);
    sigaddset(&handlermask, SIGPIPE);

    if ( sigaction(SIGHUP, &sa, NULL) == -1 ){
        perror("sigaction SIGHUP");
        exit(EXIT_FAILURE);
    }

    if ( sigaction(SIGQUIT, &sa, NULL) == -1 ){
        perror("sigaction SIGQUIT");
        exit(EXIT_FAILURE);
    }

    if ( sigaction(SIGINT, &sa, NULL) == -1 ){
        perror("sigaction SIGHUP");
        exit(EXIT_FAILURE);
    }

    if ( sigaction(SIGPIPE, &sa, NULL) == -1 ){
        perror("sigaction SIGPIPE");
        exit(EXIT_FAILURE);
    }

}



static void run_server(Parametri_server parametri) {

    struct sockaddr_un sa;

    (void)unlink(parametri.nome_socket_server);

    strcpy(sa.sun_path, parametri.nome_socket_server);
    sa.sun_family=AF_UNIX;

    int fd_client;

    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);

    if (bind(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        perror("server: bind");
        exit(EXIT_FAILURE);
    } 

    if (listen(fd_skt, SOMAXCONN) == -1) {
        perror("server: listen");
        exit(EXIT_FAILURE);
    }

    while(!parametri.isClosing && !parametri.closed) {

        if ( (fd_client = accept(fd_skt, NULL, 0)) == -1){
            perror("la socket è stata chiusa, chiusura in corso");
            break;
        } 

        JobList job = (JobList)malloc(sizeof(JobElement)); 
        job->tipo_operazione = 'X'; 
        job->fd = fd_client; 
        job->next = NULL;
        
        Pthread_mutex_lock(&mtx1);

        Push_W(coda, job);

        Pthread_mutex_unlock(&mtx1);
        pthread_cond_signal(&cond);

    }

    close(fd_skt);

}


int main(int argc, char * argv[]){


    char* nome_file = (char*)malloc(50 * sizeof(char));
    strcpy(nome_file, "config_server_test1.txt"); //se non specifico un altro nome , il file di configurazione è quello del primo test 
    if (argc == 2) strcpy(nome_file, argv[1]); 
    if (argc > 2) {
        perror("Numero di argomenti illegale");
        exit(EXIT_FAILURE);
    }

    //leggo dal file di configurazione del server 
    FILE *config;
    if ( ( config = fopen (nome_file, "r") ) == NULL ) { 
        perror("server : aprendo file di configurazione.txt");
        exit(EXIT_FAILURE);
    }

    parametri.n_file = 0; //numero di file attualmente presenti nello storage
    parametri.spazio_occupato = 0; //spazio totale dello storage attuale 
    Configura(config, &parametri);
    
    
    fclose(config); 
    free(nome_file);

    installSigHand();
    signal(SIGPIPE, SIG_IGN); //ignoro sigpipe
    
    coda = (JobList*)calloc(1,sizeof(JobList)); //coda dei job


    parametri.isClosing = 0; //dice se il server è in fase di chiusura pulita
    parametri.closed = 0; //per chiusura immediata
    

    storage = (Coda_File*)calloc(1, sizeof(Coda_File)); //DS per i file 
   
    //creo N thread workers:
    pthread_t workers[parametri.num_workers]; 
    for (int i = 0; i < parametri.num_workers; i++) {
        if (pthread_create(&workers[i], NULL, &worker , NULL) != 0 ) {
            fprintf(stderr, "creazione del thread (worker %d) fallita\n", i);
            return (EXIT_FAILURE);
        }
    }

    //SOCKET:
    run_server(parametri);


    //aspettare il ritorno dei thread
    for (int i = 0 ; i < parametri.num_workers; i++) pthread_join(workers[i], NULL);


    //chiusura di tutto 
    closeStorage(storage);
    closeListaJob(coda);
    free(coda);
    free(storage);
    free(parametri.nome_socket_server);
 


    return 0; 
}
    

   


