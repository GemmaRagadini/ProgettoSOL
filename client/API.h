#ifndef _API_H
#define _API_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <dirent.h>

/* 
    - metti errno = 0 prima di ogni funzione che lo setta 
    - errno deve essere settato opportunatamente 
    - controllare -r e -R se sono senza dirname

*/ 

#define O_CREATE 8
#define O_LOCK 4
#define NOFLAGS 0

char path[BUFSIZ];

int fd_skt;

void resetPath(){
    for (int i = 0 ; i < BUFSIZ; i++) path[i] = 0;
}


int openConnection( const char* sockname, int msec, const struct timespec abstime){

    struct sockaddr_un sa; 

    strcpy(sa.sun_path, sockname);
    sa.sun_family = AF_UNIX;

    if ( (fd_skt = socket(AF_UNIX, SOCK_STREAM, 0) ) == -1 ) {
        perror("socket"); 
        return -1;
    }

    time_t seconds_start = time(NULL);
    while( connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa) ) == -1 ) {
        if ( (difftime(time(NULL), seconds_start)) >= abstime.tv_sec){ //ho superato il tempo massimo
            perror("client : tempo per connessione superato"); //cosa vuol dire settare errno opportunamente
            return -1;
        }
     
        if (errno == ECONNREFUSED) sleep(msec / 1000); //msec dovrebbe essere in millisecondi (altrimenti cambia) 
        else {
            perror("connection failed");
            return -1;
        }
    }
    
    resetPath();
    return 0;
}

/*Chiude la connessione AF_UNIX associata al socket file sockname. Ritorna 0 in caso di successo, -1 in caso di
fallimento, errno viene settato opportunamente.*/
int closeConnection(const char* sockname){ /*non ho usato sockname, è un problema?*/

    if (sockname == NULL) {
        perror("closeConnection, socketname illegale");
        return -1;
    }
    char z = 'z'; 
    if (write(fd_skt, &z, sizeof(char)) == -1){
        perror("closeConnection: SC write");
        return -1;
    }

    if (close(fd_skt) == -1) {
        perror("close"); 
        return -1;
    }

    resetPath();

 return 0; 
}


/*Richiesta di apertura o di creazione di un file. La semantica della openFile dipende dai flags passati come secondo
argomento che possono essere O_CREATE ed O_LOCK. Se viene passato il flag O_CREATE ed il file esiste già
memorizzato nel server, oppure il file non esiste ed il flag O_CREATE non è stato specificato, viene ritornato un
errore. In caso di successo, il file viene sempre aperto in lettura e scrittura, ed in particolare le scritture possono
avvenire solo in append. Se viene passato il flag O_LOCK (eventualmente in OR con O_CREATE) il file viene
aperto e/o creato in modalità locked, che vuol dire che l’unico che può leggere o scrivere il file ‘pathname’ è il
processo che lo ha aperto. Il flag O_LOCK può essere esplicitamente resettato utilizzando la chiamata unlockFile,
descritta di seguito.
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.
*/
int openFile(const char* pathname, int flags) { 

    if (flags == O_LOCK) {
        perror("il flag O_LOCK non è supportato");
        return -1;
    }

    if (pathname == NULL || (flags != O_CREATE && flags != 0)  )  {
        perror("openFile: argomento illegale");
        return -1;
    }

    int ret ; //valore di controllo di ritorno 
    char tipo_op = 'o';

    if (write(fd_skt, &tipo_op, sizeof(char) ) == -1) {
        perror("openFIle : SC write");
        return -1;
    }

    int l = (int)(strlen(pathname));

    if (write(fd_skt, &l, sizeof(int)) == -1) {
        perror("openFile: SC write");
        return -1; 
    }

    if (write(fd_skt, pathname, l+1 )== -1 ) {
        perror("openFile: SC write");
        return -1; 
    }

    if ( write(fd_skt, &flags, sizeof(int ) ) == -1)  {
        perror("openFile: SC write");
        return -1; 
    }
    

    if (read(fd_skt, &ret, sizeof(int))== -1){ 
        perror("openFile: SC read");
        return -1;
    } 
        
    strcpy(path, pathname);

    return ret;
}




/*Legge tutto il contenuto del file dal server (se esiste) ritornando un puntatore ad un'area allocata sullo heap nel
parametro ‘buf’, mentre ‘size’ conterrà la dimensione del buffer dati (ossia la dimensione in bytes del file letto). In
caso di errore, ‘buf‘e ‘size’ non sono validi. Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene
settato opportunamente.*/
//se non specificato -d dirname la readFile legge ma non salva il contenuto da nessuna parte 
int readFile(const char* pathname, void** buf, size_t* size){ 
     if (pathname == NULL || buf == NULL || size == NULL) {
        perror("readFile: argomento illegale");
        return -1;
    }

    int ret;
    char tipo_op = 'r';

    
    size = (size_t*)malloc(sizeof(size_t)); //boh 

    if ( write(fd_skt, &tipo_op, sizeof(char)) == -1) {
        perror("readFile: SC write");
        return -1;
    }

    long l = strlen(pathname);

    if (write(fd_skt, &l, sizeof(int)) == -1) {
        perror("readFile: SC write");
        return -1;
    }

    if (write(fd_skt, pathname,  l +1) == -1) {
        perror("readFile: SC write");
        return -1;
    }

    if (read(fd_skt, &ret, sizeof(int)) == -1){
        perror("readFile: SC read");
        return -1;
    }

    if (ret == -1) return -1;

    if (read(fd_skt, size, sizeof(size_t)) == -1) { // va bene il sizeof?
        perror("readFile: SC read");
        return -1;
    }

    char * buffer = (char*)malloc(( *size +1) * sizeof(char));

    if (read(fd_skt, buffer, *size + 1) == -1){ 
        perror("readFile: SC read");
        return -1;
    }

    strcpy(*buf, buffer);


    

    resetPath();

 return ret;

 }




/*Richiede al server la lettura di ‘N’ files qualsiasi da memorizzare nella directory ‘dirname’ lato client. Se il server
ha meno di ‘N’ file disponibili, li invia tutti. Se N<=0 la richiesta al server è quella di leggere tutti i file
memorizzati al suo interno. Ritorna un valore maggiore o uguale a 0 in caso di successo (cioè ritorna il n. di file
effettivamente letti), -1 in caso di fallimento, errno viene settato opportunamente.*/
int readNFiles(int N, const char* dirname) {
    if (dirname == NULL) {
        perror("readNFiles: argomento illegale");
        return -1;
    }


    char tipo_op = 'R';

    if (write(fd_skt, &tipo_op, sizeof(char)) == -1){
        perror("readNFiles: SC write");
        return -1;
    }

    if (write(fd_skt, &N, sizeof(int)) == -1){
        perror("readNFiles: SC write");
        return -1;
    }

    int numero_letti;
    if (read(fd_skt, &numero_letti, sizeof(int) ) == -1) {
        perror("readNFiles: SC read");
        return -1;
    }

    if (numero_letti < 0 ) {
        errno = numero_letti;
        return -1;
    }

    if (numero_letti == 0) {
        printf("readNFiles: lo storage è vuoto\n");
        return 0;
    }

    int cont = 0;
    int l_pathname;
    int l_buffer;
    FILE* fd;

    while (cont < numero_letti) {
        if (read(fd_skt, &l_pathname, sizeof(int)) == -1) {
            perror("readNFiles: SC read lunghezza pathname");
            return -1;
        }


        char *pathname = (char*)malloc((l_pathname+1) * sizeof(char));
        if (read(fd_skt, pathname, l_pathname+1) == -1) {
            perror("readNFiles: SC read pathname");
            return -1;
        }
     
        //estraggo il nome della stringa dal pathname
        char * tmpstr;
        char * str = strtok_r(pathname, "/", &tmpstr); 
        char * nomefile;
        while (str != NULL){
            nomefile= str; 
            str = strtok_r(NULL, "/", &tmpstr );
        }


        if (read(fd_skt, &l_buffer, sizeof(int)) == -1) {
            perror("readNFiles: SC read lunghezza buffer ");
            return -1;
        }


        char * buffer = (char*) malloc((l_buffer+1) * sizeof(char));
        if (read(fd_skt, buffer, l_buffer+1) == -1) {
            perror("readNFiles: SC read buffer");
            return -1;
        }

        char * nomecompleto = (char*)malloc((strlen(dirname)+ strlen(nomefile)+ 1)*sizeof(char));
        strcpy( nomecompleto,dirname);
        nomecompleto[strlen(nomecompleto)-1] = '\0';
        strcat(nomecompleto,"/");
        strcat(nomecompleto,nomefile);

        fd = fopen(nomecompleto , "w");

        if (fputs(buffer,fd) == -1) {
            perror("readNFiles: errore nella scrittura del file nella directory indicata");
            return -1;
        }

        if (fclose(fd) == -1) {
            perror("readNFiles: SC close fallita");
            return -1;
        }

        free(pathname);
        free(buffer);
        cont ++;
    }
    int ret;
    if (read(fd_skt, &ret, sizeof(int)) == -1) {
        perror("readNFiles: SC read valore di ritorno");
        return -1;
    }


    resetPath();

    return ret;

}



/*Scrive tutto il file puntato da pathname nel file server. Ritorna successo solo se la precedente operazione,
terminata con successo, è stata openFile(pathname, O_CREATE| O_LOCK). Se ‘dirname’ è diverso da NULL, il
file eventualmente spedito dal server perchè espulso dalla cache per far posto al file ‘pathname’ dovrà essere
scritto in ‘dirname’; Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.*/
int writeFile(const char* pathname, const char* dirname){


    //condizione affinché l'ultima operazione effettuata con successo sia stata la open , su pathname 
    if (strcmp(path,pathname) != 0 ) {
        perror("L'ultima operazione effettuata con successo non è stata la open su questo file");
        return -1;
    }

    if (pathname == NULL) {
        perror("writeFile: argomento illegale");
        return -1;
    }

    if (dirname != NULL) {
        perror("writeFile con dirname!=NULL non è supportata");
        return -1;
    }

    int ret; 
    char tipo_op = 'W';

    FILE *file;
    if ( ( file = fopen(pathname, "r") ) == NULL) {
        perror("writeFile: fopen");
        return -1;
    }

    //cerco dimensione del file 
    fseek(file, 0, SEEK_END);
    size_t dim = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    
    char * buffer = (char*)malloc( dim * sizeof(char));

    if (fread(buffer,sizeof(char), dim, file) != dim ) { 
        perror("WriteFile :fread");
        return -1;
    }


    if (write(fd_skt, &tipo_op, sizeof(char) ) == -1){
        perror("writeFile: write del tipo di operazione");
        return -1;
    }

    int l_path = (int)( strlen(pathname) );

    if (write(fd_skt, &l_path, sizeof(int)) == -1) {
        perror("writeFile: write lunghezza del pathname");
        return -1;
    }

    if (write(fd_skt, pathname, l_path+1 ) == -1){
        perror("writeFile: write pathname");
        return -1;
    }

    if ( write(fd_skt, &dim, sizeof(size_t)) == -1) {
        perror("writeFile: write lunghezza del buffer");
        return -1;
    }

    if (write(fd_skt, buffer, dim) == -1) {
        perror("writeFile: write buffer");
        return -1;
    }

    if (read(fd_skt, &ret, sizeof(int) ) == -1){
        perror("writeFile:read valore di ritorno");
        return -1;
    }

    resetPath();

 return ret;
}


/*int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname)
Richiesta di scrivere in append al file ‘pathname‘ i ‘size‘ bytes contenuti nel buffer ‘buf’. L’operazione di append
nel file è garantita essere atomica dal file server. Se ‘dirname’ è diverso da NULL, il file eventualmente spedito
dal server perchè espulso dalla cache per far posto ai nuovi dati di ‘pathname’ dovrà essere scritto in ‘dirname’;
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){ //anche in questo caso ignoro dirname

    char tipo_op = 'a';
    int ret; 

    if (write(fd_skt, &tipo_op, sizeof(char)) == -1) {
        perror("appendToFile: SC write");
        return -1;
    }

    int l = (int)(strlen(pathname));

    if (write(fd_skt, &l, sizeof(int)) == -1) {
        perror("appendToFile: SC write");
        return -1;
    }

    if (write(fd_skt, pathname, l+1) == -1) {
        perror("appendToFile: SC write");
        return -1;
    }

    size = strlen(buf);

    if (write(fd_skt, &size, sizeof(size_t)) == -1) {
        perror("appendToFile : SC read");
        return -1;
    }

    if (write(fd_skt, buf , size +1 ) == -1 ) { //boh 
        perror("appendToFile : SC read");
        return -1;
    }

    if (read(fd_skt, &ret, sizeof(int) ) == -1) {
        perror("appendToFile : SC read");
        return -1;
    }

    resetPath();

 return ret;
}



/*
Richiesta di chiusura del file puntato da ‘pathname’. Eventuali operazioni sul file dopo la closeFile falliscono.
Ritorna 0 in caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.*/
int closeFile(const char* pathname){
    char tipo_op = 'c';
    int ret; 

    if(write(fd_skt, &tipo_op, sizeof(char)) == -1) {
        perror("closeFile : SC write");
        return -1;
    }

    int l = (int)(strlen(pathname));

    if (write(fd_skt, &l, sizeof(int)) == -1) {
        perror("closeFile : SC write");
        return -1;
    }

    if (write(fd_skt, pathname, l+1) == -1) {
        perror("closeFile : SC write");
        return -1;
    }

    if (read(fd_skt, &ret, sizeof(int)) == -1) {
        perror("closeFile : SC read");
        return -1;
    }

    resetPath();
    
    return ret;
}


/*In caso di successo setta il flag O_LOCK al file. Se il file era stato aperto/creato con il flag O_LOCK e la
richiesta proviene dallo stesso processo, oppure se il file non ha il flag O_LOCK settato, l’operazione termina
immediatamente con successo, altrimenti l’operazione non viene completata fino a quando il flag O_LOCK non
viene resettato dal detentore della lock. L’ordine di acquisizione della lock sul file non è specificato. Ritorna 0 in
caso di successo, -1 in caso di fallimento, errno viene settato opportunamente.*/
int lockFile(const char * pathname){
    perror("lockFile non supportata");
    return -1;
}



/*Resetta il flag O_LOCK sul file ‘pathname’. L’operazione ha successo solo se l’owner della lock è il processo che
ha richiesto l’operazione, altrimenti l’operazione termina con errore. Ritorna 0 in caso di successo, -1 in caso di
fallimento, errno viene settato opportunamente.*/
int unlockFile(const char * pathname){
    perror("unlockFile non supportata");
    return -1;
}




/*Rimuove il file cancellandolo dal file storage server. L’operazione fallisce se il file non è in stato locked, o è in
stato locked da parte di un processo client diverso da chi effettua la removeFile.*/ //LA DEVO FARE ?
int removeFile(const char* pathname){
    perror("removeFile non supportata");
    return -1;
}

#endif




