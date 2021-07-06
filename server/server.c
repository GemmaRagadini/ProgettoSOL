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
#include "works.h"
#include "mutex.h"
#include "tok_s.h"
#include "file.h"
#include "configura_server.h"


int fd_skt; 

JobList coda; //coda di jobs che devono essere eseguiti


Coda_File storage; //struttura dati per i file 

Parametri_server parametri; //struttura in cui stanno i parametri presi dal file di configurazione


static pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER; //per la signal sulla coda di work 
static pthread_mutex_t mtx1 = PTHREAD_MUTEX_INITIALIZER; //mutex per la coda di work 
static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER; //mutex per la DS 


#define O_CREATE 8
#define O_LOCK 4 
#define NOFLAGS 0



/* 
    -per ora nessuno attende il ritorno dei thread
    - mancano i segnali per far terminare il server 
    - crea un thread che controlla la terminazione (sigwait) 
    - va deciso come e se far terminare i client e come avvisare il server 
    - manca ancora totalmente la terminazione di tutto 
    - il client e il server condividono tok.h-> decidi se lasciarlo e motivarlo nella relazione o farne due copie (con nomi diversi) di entrambi 
    - richiesta - risposta 
    - master-workers 
    - SE IL CLIENT SBAGLIA IL SERVER SI FERMA! -> stampando z , non capisco come possa fermarsi?
    -gitHUb?
    - la write sembrerebbe funzionare ma non ha un'opzione per testarla direttamente 
    - devo fare una libreria ? 
    - se il client non fa la closeFile e poi la closeConnection , il server cerca di ripetere l'ultima operazione e fallisce  ovviamente
    - non ho testato quando si riempie lo spazio del server

*/

int Open(const char * pathname,int fd, int flags) {

    if (pathname == NULL) {
        perror("Open: pathname illegale, server\n");
        return -1;
    }


    switch (flags) {
        case O_CREATE|O_LOCK:;
        case O_CREATE: 
                if (cerca_f(storage, pathname) == 1) {
                printf("Open con O_CREATE di un file già presente nello sorage\n");
                return -1;
            }

            if (push_f(&storage, pathname, fd, &parametri) == -1) {
                perror("Open: errore nel caricamento del file, server\n");
                return -1;
            }

            return 0;

        case O_LOCK:;
        case 0: 

            if (cerca_f(storage, pathname) == -1) {
                printf("open senza O_CREATE di un file non presente nello storage\n");
                return -1;
            }

             //se è già stato aperto lo controlla file.h
            if (open_c(&storage, pathname, fd) == -1 ) {
                perror("Open: errore nell'apertura del file , server");
                return -1;
            }
         
            return 0;
        default:
            perror("openFile: flag illegale,server\n");
            return -1;
    }
}



int Write(const char * pathname, int fd) { //funziona ovviamente solo nel caso di un file esistente nel FS -> da capire come gestire altrimenti 


    if (pathname == NULL) {
        perror("Write : pathname illegale, server");
        return -1;
    }

    if (cerca_f(storage,pathname) == -1) {
        perror ("Write: il file non è presente nello storage");
        return -1;
    }

    if (isOpened(storage,pathname,fd) == -1) {
        perror("Write : il file non è stato aperto dal client");
        return -1;
    }

    size_t dim; 
    if (read(fd, &dim, sizeof(size_t)) == -1) {
        perror("Write: read della dimensione del file fallita");
        return -1;
    }

    char * buffer = (char*)malloc( dim * sizeof(char));
    if (read(fd, buffer, dim) == -1) {
        perror("Write: read del contenuto del file fallita");
        return -1;
    }
  
    if (write_f(&storage, pathname, buffer, &parametri) == -1) { 
        perror ("Write: scrittura non andata a buon fine , server");
        return -1;
    }

    printf("%s\n", contenuto_f(storage, pathname));
    

    return 0;
}



int Close(const char * pathname, int fd) {

    if (pathname == NULL) {
        perror("Write : pathname illegale, server");
        return -1;
    }

    if (close_f(&storage,pathname, fd) == -1) {
        perror("Close: errore in chiusura, server");
        return -1;
    }

    return 0;
}




int Read(const char * pathname, int fd, char ** buf) {

    if (pathname == NULL) {
        perror("Read : pathname illegale, server");
        return -1;
    }

    if (cerca_f(storage, pathname) == -1) {
        perror("Read: il file non è presente nello storage, server");
        return -1;
    }

    if (isOpened(storage, pathname, fd) == -1 ) {
        perror("Read: il file non è stato aperto dal client, server");
        return -1;
    }
    
    if ( ( *buf = contenuto_f(storage, pathname ) ) == NULL) return -1;

    
    return 1; //è grande BUFSIZ, serve di più?
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

    if (isOpened(storage, pathname, fd) == -1) { 
        perror("Append: il file non è stato aperto dal client, server");
        return -1;
    }

    if (append(&storage, pathname, buf, &parametri) == -1) {
        perror("Append: scrittura fallita, storage");
        return -1;
    }

    return 1;
}



int readN(int N, int fd){
    int n = conta_f(storage); 
    if (n == 0) return n;  
    if (N <= 0) N = n;
    if (n > N) n = N;
    //if (n < 0) li legge automaticamente tutti direi 
    //adesso in ogni caso ne leggo n 
    int cont = 0; 
    int l_pathname; 
    int l_buffer; 
    char * buffer;
    Coda_File corrente = storage;
    if (write(fd, &n, sizeof(int)) == -1) {
        perror("readN : SC read numero letti, server");
        return -1; //così li sballo tutti , è troppo (anche se eventualmente altri fossero andati a buon fine -> si può verificare questo caso  ?)
    }

    while (cont < n && corrente != NULL) {

        if (contenuto_f(storage, corrente->pathname) != NULL) { //forse questo non serve 
            buffer = contenuto_f(storage, corrente->pathname);
            l_pathname = (int)strlen(corrente->pathname);
            l_buffer = (int)strlen(buffer);
          
            if (write(fd, &l_pathname, sizeof(int)) == -1) {
                perror("readN : SC read lunghezza pathname, server");
                return -1; //così li sballo tutti , è troppo (anche se eventualmente altri fossero andati a buon fine -> si può verificare questo caso  ?)
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
    
    while(1) {
        Pthread_mutex_lock(&mtx1);

        while (coda == NULL) pthread_cond_wait(&cond, &mtx1); 

        JobList estratto = (JobList)malloc(sizeof(JobElement));
        estratto = Pop_W(&coda);
        Pthread_mutex_unlock(&mtx1);
        
        int ret; //valore da ritornare al client 

        int l; //grandezza del pathname

        int N; //numero file da leggere nel caso R 
        
        char * buf = (char*)malloc(BUFSIZ * sizeof(char));

        size_t size;

        if (estratto->tipo_operazione == 'X') {
            if (read(estratto->fd, &(estratto->tipo_operazione), sizeof(char)) <= 0){
                perror("worker : SC read fd, server\n");
                continue;
            }
        }
        printf("%c\n", estratto->tipo_operazione);

        char* pathname = (char*)malloc(BUFSIZ * sizeof(char));


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


                if ( Read(pathname, estratto->fd, &buf )== -1) {
                    perror("worker: Read fallita, server");
                    ret = -1;
                }

                Pthread_mutex_unlock(&mtx2);
                size = strlen(buf);
                if (write(estratto->fd, &size, sizeof(size_t) ) == -1 ) {
                    perror("worker : SC write size, server");
                    ret = -1; //non serve a niente in questo caso penso 
                    break;
                }

                if (write(estratto->fd, buf, size +1) == -1) { 
                    perror ("worker : SC write buf, server");
                    ret = -1;
                    break;
                }

                ret = 1;

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
            case 'W' : //forse questo non lo devo mettere 

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
                    
                    if (read(estratto->fd, buf, size +1) == -1) {
                        perror("worker, SC read buffer, server");  //forse ci aggiungerei di scrivere in che caso sono
                        ret = -1;
                        break;
                    }
                    Pthread_mutex_lock(&mtx2);
                    ret = Append(pathname, estratto->fd, buf, size);
                    Pthread_mutex_unlock(&mtx2);

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

        if (write(estratto->fd, &ret, sizeof(int)) <= 0) {
            perror("worker:SC write ret, server\n");
            break; //lo devo gestire con i segnali penso 
        }

        char k;
        int fd = estratto->fd;
        free(estratto);

        if (read (fd, &k, sizeof(char)) != -1 ){ // e si è scollegato accidentalmente? 
            if (k != 'z') { // se è z vuol dire che il client si sta per disconnettere e non manderà più richieste 
                JobList job = (JobList)malloc(sizeof(JobElement)); 
                job->tipo_operazione = k; 
                job->fd = fd;
                job->next = NULL;
            

                Pthread_mutex_lock(&mtx1);
                Push_W(&coda, job);
                pthread_cond_signal(&cond); //boh ? 
                Pthread_mutex_unlock(&mtx1); 
            }
        }
     
    }

    return (void*)0;
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

    while(1){

        if ( (fd_client = accept(fd_skt, NULL, 0)) == -1){
            perror("server: accept");
            break;
        } 

        JobList job = (JobList)malloc(sizeof(JobElement)); 
        job->tipo_operazione = 'X'; 
        job->fd = fd_client; 
        job->next = NULL;
        
        Pthread_mutex_lock(&mtx1);

        Push_W(&coda, job);

        Pthread_mutex_unlock(&mtx1);
        pthread_cond_signal(&cond);
        //stampaLista(storage);


    }
}

    

int main(){
   
    
    //leggo dal file di configurazione del server 
    FILE *config;
    if ( ( config = fopen ("config_server.txt", "r") ) == NULL ) { //apertura file di configurazione
        perror("server : aprendo config_server.txt");
        exit(EXIT_FAILURE);
    }

    parametri.n_file = 0; //numero di file attualmente presenti nello storage
    parametri.spazio_occupato = 0; //spazio totale dello storage attuale 
    Configura(config, &parametri);
    
    
    fclose(config); 

    
    coda = (JobList)malloc(sizeof(JobElement)); //coda dei job
    coda = NULL;
    

    storage = (Coda_File)malloc(sizeof(File)); //DS per i file 
    storage = NULL;
   

    //creo N thread workers:
    pthread_t workers[parametri.num_workers]; 
    for (int i = 0; i < parametri.num_workers; i++) {
        if (pthread_create(&workers[i], NULL, &worker , NULL) != 0 ) {
            fprintf(stderr, "pthread_create failed (worker %d)\n", i);
            return (EXIT_FAILURE);
        }
    }

    //stampaLista(storage);
    //SOCKET:
    run_server(parametri);


    close(fd_skt);
    

    return 0; 
}
    

   


