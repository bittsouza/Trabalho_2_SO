
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


#define NBLOCKS 10
#define BLOCK_SIZE 36
#define MAX_ARQ 2
    
// Funções
    int create_socket();
    void* comando(void* connfd_);
    int fylesystem();
    void root();
    int find_block();
    int find_inode();
    int mkdir_(char* recvBuffer);
    int rm_r(char* recvBuffer);
    char* ls();
    int touch(char* recvBuffer);
    int rm(char* recvBuffer);
    char* echo(char* recvBuffer);
    char* cat(char* recvBuffer);


// Structs
    struct inode_{
        char name[5];   // Name arc/dir
        char type[3];   // Type arc/dir
        int block;      // Block referente ao arc/dir
        bool state;     // state
    };
    typedef struct inode_ inode;


    struct data_{   
        char name[5];
        int inode;
    };  
    typedef struct data_ data;


    struct dir_{
        int current;
        int prev;
        data arq[MAX_ARQ];
    };
    typedef struct dir_ dir;

    // Variáveis globais
    pthread_mutex_t mutex;

    int map_size = sizeof(bool) * NBLOCKS;
    int inode_size = sizeof(inode) * NBLOCKS;
    int block_size = BLOCK_SIZE * NBLOCKS;
    int dir_current;

    FILE *file;



int fylesystem(){
    file = fopen("fs.bin", "w+b");
    rewind(file);

    
    bool state = false; // Bit map, mapa de espaço livre
    for(int i = 0; i < NBLOCKS; i++){
        fwrite(&state, sizeof(bool), 1, file); // Escreve state do arquivo
    }  

    // Inicializa inode
    inode temp;
    strcpy(temp.name, "none");
    strcpy(temp.type, "nan");
    temp.block = -1;
    temp.state = false;
    
    // Encontra primeiro inode vazio
    fseek(file, map_size, SEEK_SET);
    for(int i = 0; i < NBLOCKS; i++){
        
        fwrite(&temp, sizeof(inode), 1, file);  //escrevendo no inode
    }

    
    char *temp_ = malloc(BLOCK_SIZE); //seta o block com o tamanho pre definido
    
    fseek(file, (map_size + inode_size), SEEK_SET); //pula bitmap

    for(int i = 0; i < NBLOCKS; i++){
        
        fwrite(temp_, BLOCK_SIZE, 1, file);   //escreve block vazio
    }

    
    root(); 

    return 0;
}

void root(){
    inode temp;
    bool state = 1;

    fseek(file, map_size, SEEK_SET);

    fread(&temp, sizeof(inode), 1, file);

    strcpy(temp.name, "Raiz"); 
    strcpy(temp.type, "Dir");  
    temp.block = find_block(); 
    temp.state = 1;

    fseek(file, map_size, SEEK_SET); 

    fwrite(&temp, sizeof(inode), 1, file); //escreve a var temporario na localização do primeiro inode do arquivo

    dir temp_; 
    temp_.current = 0;
    temp_.prev = 0;
    dir_current = 0;

    
    fseek(file, (map_size + inode_size + temp.block * BLOCK_SIZE), SEEK_SET);   
    
    fwrite(&temp_, sizeof(temp_), 1, file);   //screve o diretorio vazio na localizacao do block do arquivo

    return;
}

int create_socket(){
    struct sockaddr_in socketAddr;

    
    socketAddr.sin_family = AF_INET;                        
    socketAddr.sin_port = htons(5000);                      
    socketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(&(socketAddr.sin_zero), 8);


    int socket_read = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_read == -1){
        printf ("Erro ao criar o socket.\n");
        exit(-1);
    }

    int enable = 1;
    setsockopt(socket_read, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    int bind_read = bind(socket_read, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
    if(bind_read == -1){
        printf("Erro encontrado na funcao bind.\n");
        exit(-1);
    }


    int listen_read = listen(socket_read, 10);
    if(listen_read == -1){
        printf("Erro encontrado na funcao listen.\n");
        exit(-1);
    }

    return socket_read;
}


int find_block(){

    fseek(file, 0, SEEK_SET);

    bool state;
    int i;

    
    for(i = 0; i < NBLOCKS; i++){
        fread(&state, sizeof(bool), 1, file);
        if(state == 0)
            break;
    }

    // Com o block livre, muda o state para ocupado
    state = 1;
    fseek(file, (i * sizeof(bool)), SEEK_SET);
    fwrite(&state, sizeof(bool), 1, file);   

    return i;

}

int find_inode(){

    
    inode temp;
    int i;

    // Busca o primeiro inode livre
    for(i = 0; i < NBLOCKS; i++){
        fseek(file, (map_size + i * sizeof(inode)), SEEK_SET);
        fread(&temp, sizeof(inode), 1, file);
        if(temp.state == 0)
            break;
    }

    // Com o inode livre, altera o state para ocupado
    temp.state = 1;
    fseek(file, (map_size + i * sizeof(inode)), SEEK_SET);
    fwrite(&temp, sizeof(inode), 1, file);

    return i;
}

void* comando(void* connfd_){
    int* conexao = (int*)connfd_;
    char sendBuffer[1024], recvBuffer[1024];

    memset(sendBuffer, 0, sizeof(sendBuffer));

    strcpy(sendBuffer, "Conexao realizada com sucesso!\n");
    send(*conexao, sendBuffer, strlen(sendBuffer), 0);

    while (strcmp(recvBuffer, "exit") != 0){

        memset(recvBuffer, 0, sizeof(recvBuffer));
        int recvR = recv(*conexao, recvBuffer, 100, 0);

        //Adiciona um \0 ao final da string para indicar seu termino
        recvBuffer[recvR - 1] = '\0'; 
        int status;

        if (strncmp(recvBuffer, "mkdir ", 6) == 0){
            pthread_mutex_lock(&mutex);

            status = mkdir_(recvBuffer);

            if(!status) printf("Pasta criada com sucesso\n");
            else printf ("Erro ao criar pasta\n");
        
            pthread_mutex_unlock(&mutex);
        }

        
        if (strncmp(recvBuffer, "rm -r ", 6) == 0){
            pthread_mutex_lock(&mutex);
            
            if(!status) printf("Pasta excluida com sucesso\n");
            else printf ("Erro ao excluir pasta\n");

            pthread_mutex_unlock(&mutex);

        }
         
        if (strncmp(recvBuffer, "cd ", 3) == 0){

            pthread_mutex_lock(&mutex);            
            memmove(recvBuffer, recvBuffer + 3, strlen(recvBuffer)); 

            chdir(recvBuffer);
            printf("Entrando no diretorio %s\n", recvBuffer);
            pthread_mutex_unlock(&mutex);
        }

        if (strcmp(recvBuffer, "ls") == 0){

            char *buffer = ls();

            send(*conexao, buffer, strlen(buffer), 0);

        }

        if (strncmp(recvBuffer, "touch ", 6) == 0){
            pthread_mutex_lock(&mutex);

            status = touch(recvBuffer);

            if(!status) printf("Arquivo criado com sucesso\n");
            else printf ("Erro ao criar arquivo\n");

            pthread_mutex_unlock(&mutex);
        }

        if ((strncmp(recvBuffer, "rm ", 3) == 0) && (strncmp(recvBuffer, "rm -r ", 6) != 0) ){
            pthread_mutex_lock(&mutex);

            if(!status) printf("Arquivo removido com sucesso\n");
            else printf("Erro ao remover arquivo\n");

            pthread_mutex_unlock(&mutex);
        }

        if (strncmp(recvBuffer, "echo ", 5) == 0){
            pthread_mutex_lock(&mutex);

            if(!status) printf("Caracteres inseridos com sucesso\n");
            else printf("Erro ao inserir caracteres\n");

            pthread_mutex_unlock(&mutex);
        }

        if (strncmp(recvBuffer, "cat ", 4) == 0){
            pthread_mutex_lock(&mutex);

            send(*conexao, sendBuffer, strlen(sendBuffer), 0);
            
            pthread_mutex_unlock(&mutex);
        }

    }
    printf("Conexao fechada\n");
}

char* ls(){
    
    inode temp;
    int block;
    dir current;
    char *list = (char*) malloc(sizeof(char));

    fseek(file, (map_size + dir_current * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);

    block = temp.block;

    fseek(file, (map_size + inode_size + block * BLOCK_SIZE), SEEK_SET);
    fread(&current, BLOCK_SIZE, 1, file);
        
    strcat(list, "\t.\t..");

    for(int i = 0; i < MAX_ARQ; i++){
        if(current.arq[i].name != NULL){
            strcat(list, "\t");
            strcat(list, current.arq[i].name);
        }
        else break;
    }

    list[strlen(list) - 2] = '\n';
    list[strlen(list) - 1] = '\0';

    return list;

}

int touch(char* recvBuffer){

    int block_livre, block_dir;
    inode temp;
    dir current;
    char *name;

    block_livre = find_block();

    fseek(file, (map_size + dir_current * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);

    block_dir = temp.block;

    fseek(file, (map_size + inode_size + block_dir * BLOCK_SIZE), SEEK_SET);
    fread(&current, BLOCK_SIZE, 1, file);

    name = strtok(recvBuffer," ");
    name = strtok(NULL, " ");

    int i;
    for(i = 0; i < MAX_ARQ; i++){
        if(current.arq[i].inode != 0)
            break;
    }

    if(i != MAX_ARQ){
        strcpy(current.arq[i].name, name);
        current.arq[i].inode = block_livre;
    }
    else{
        printf("Diretorio cheio\n");
        exit(0);
    }

    fseek(file, (map_size + inode_size + block_dir * BLOCK_SIZE), SEEK_SET);
    fwrite(&current, sizeof(dir), 1, file);

    return 0;
    
}

int mkdir_(char* recvBuffer){

    inode temp;
    int inode_pos, status;
    bool state = 1;
    char *name;

    inode_pos = find_inode();
    
    fseek(file, (map_size + inode_pos * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);
     
     // Retira o "mkdir " do comando
    name = strtok(recvBuffer," ");
    name = strtok(NULL, " ");
    
    // Insere os dados no inode
    strcpy(temp.name, name);
    strcpy(temp.type, "Dir");
    temp.block = find_block();

    dir temp_;
    temp_.current = inode_pos;
    temp_.prev = dir_current;

    fseek(file, (map_size + inode_size + temp.block * BLOCK_SIZE), SEEK_SET);    
    fwrite(&temp_, sizeof(temp_), 1, file);   
    
    status = 0;
    
    return status;
}

int main(){

    int socket_read = create_socket();

    printf("Socket criado\n");
    
    int initialize = fylesystem();
    if (initialize != 0){
        printf("Erro ao criar sistema de arquivo\n");    
        exit(-1);
    }
    printf("Sistema de Arquivo criado com sucesso\n");

    while(1){
        int *connfd = calloc(sizeof(int), 1);

        printf("\nEsperando conexao\n");        
        *connfd = accept(socket_read, (struct sockaddr*)NULL, NULL);

        if(*connfd == -1){
            printf("Erro ao aceitar a conexao\n");
            exit(-1);
        }
        printf("Conexao criada com sucesso\n");

        pthread_t tid;
        pthread_create(&tid, NULL, comando, connfd);
        printf("Thread criada.\n");

    }

    return 0;
}