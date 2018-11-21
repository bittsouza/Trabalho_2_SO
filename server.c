
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

    

// Variáveis globais
    pthread_mutex_t cadeado;

    int map_size = sizeof(bool) * NBLOCKS;
    int inode_size = sizeof(inode) * NBLOCKS;
    int block_size = BLOCK_SIZE * NBLOCKS;
    int dir_atual;

    
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
        int block;      // Bloco referente ao arc/dir
        bool state;     // state
    };
    typedef struct inode_ inode;


    struct data_{   
        char name[5];
        int inode;
    };  
    typedef struct dado_ dado;


    struct dir_{
        int now;
        int prev;
        data arq[MAX_ARQ];
    };
    typedef struct dir_ dir;

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
    strcpy(temp name, "none");
    strcpy(temp.type, "nan");
    temp.block = -1;
    temp.state = false;
    
    // Encontra primeiro inode vazio
    fseek(file, map_size, SEEK_SET);
    for(int i = 0; i < NBLOCKS; i++){
        
        fwrite(&temp, sizeof(inode), 1, file);  //escrevendo no inode
    }

    
    char *temp_ = malloc(BLOCK_SIZE); //seta o bloco com o tamanho pre definido
    
    fseek(file, (map_size + inode_tam), SEEK_SET); //pula bitmap

    for(int i = 0; i < NBLOCKS; i++){
        
        fwrite(temp_, BLOCK_SIZE, 1, file);   //escreve bloco vazio
    }

    
    root(); 

    return 0;
}

void root(){
    
    //cria diretorio raiz
    inode temp;
    bool state = 1;

    // Pula para o primeiro inode
    fseek(file, map_size, SEEK_SET);

    // Le o primeiro inode e insere na variavel temp
    fread(&temp, sizeof(inode), 1, file);

    strcpy(temp name, "Raiz"); //esceve nome
    strcpy(temp.type, "Dir");  //escreve diretorio
    temp.block = find_block(); 
    temp.state = 1;

    fseek(file, map_size, SEEK_SET); //encontra primeiro inode livre

    
    fwrite(&temp, sizeof(inode), 1, file); //escreve a var temp na localização do primeiro inode do arquivo

    dir temp_; //dir vazio
    temp_.atual = 0;
    temp_.ant = 0;
    dir_atual = 0;

    
    fseek(file, (map_size + inode_tam + temp.block * BLOCK_SIZE), SEEK_SET);   //encontra primeiro bloco livre
    
    fwrite(&temp_, sizeof(temp_), 1, file);   //screve o diretorio vazio na localizacao do bloco do arquivo

    return;
}

int find_block(){
    // Pula para o inicio do arquivo
    fseek(file, 0, SEEK_SET);

    bool state;
    int i;

    // Procura no bit map o primeiro bloco livre
    for(i = 0; i < NBLOCKS; i++){
        // Le a variavel de state do bit map
        fread(&state, sizeof(bool), 1, file);
        // Verifica se o bloco esta livre
        if(state == 0)
            break;
    }

    // Se o bloco estiver livre, muda o state para ocupado
    state = 1;
    fseek(file, (i * sizeof(bool)), SEEK_SET);
    fwrite(&state, sizeof(bool), 1, file);   

    return i;

}

int create_socket(){
    struct sockaddr_in socketAddr;

    //Configuracao do socket
	socketAddr.sin_family = AF_INET;						// Familia do endereço
	socketAddr.sin_port = htons(7000);						// Porta escolhida (htons converte o valor na ordem de bytes da rede)
	socketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	// Endereço IP, localhost (inet_addr converte o endereço para binario em bytes)
	bzero(&(socketAddr.sin_zero), 8);						// Prevencao de bug para arquiteturas diferentes


	/* Inicializa o socket:
    Dominio da comunicacao (AF_NET p/ TCP/IP),
    type de comunicacao (TCP/UDP) (SOCK_STREAM p/ TCP)
    Protocolo (0) */
	int socketR = socket(AF_INET, SOCK_STREAM, 0);
	if(socketR == -1){
		printf ("Erro ao criar o socket.\n");
		exit(-1);
	}

    int enable = 1;
    setsockopt(socketR, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

	//Associa o socket criado com a porta do SO
	int bindR = bind(socketR, (struct sockaddr *)&socketAddr, sizeof(socketAddr));
	if(bindR == -1){
		printf("Erro na funcao bind.\n");
		exit(-1);
	}

    //Habilita o socket pra receber as conexoes atraves do descritor (socketR) do socket
	int listenR = listen(socketR, 10);
	if(listenR == -1){
		printf("Erro na funcao listen.\n");
		exit(-1);
	}

	return socketR;
}


void* comando(void* connfd_){
    int* conexao = (int*)connfd_;
    char sendBuffer[1024], recvBuffer[1024];

    memset(sendBuffer, 0, sizeof(sendBuffer));

    strcpy(sendBuffer, "Conexao realizada com sucesso!\n");
    send(*conexao, sendBuffer, strlen(sendBuffer), 0);

    while (strcmp(recvBuffer, "exit") != 0){

        //Retorna o tamanho da String que o cliente escreveu
        memset(recvBuffer, 0, sizeof(recvBuffer));
        int recvR = recv(*conexao, recvBuffer, 100, 0);

        //Adiciona um \0 ao final da string para indicar seu termino
        recvBuffer[recvR - 1] = '\0'; 
        int status;

        //criar (sub)diretorio
        if (strncmp(recvBuffer, "mkdir ", 6) == 0){
            pthread_mutex_lock(&cadeado);

            status = mkdir_(recvBuffer);

            if(!status) printf("Pasta criada com sucesso\n");
            else printf ("Erro ao criar pasta\n");
        
            pthread_mutex_unlock(&cadeado);
        }

        //remover (sub)diretorio
        if (strncmp(recvBuffer, "rm -r ", 6) == 0){
            pthread_mutex_lock(&cadeado);

            //status = rm_r(recvBuffer);
            
            if(!status) printf("Pasta excluida com sucesso\n");
            else printf ("Erro ao excluir pasta\n");

            }
         //entrar em (sub)diretorio
        
     // Bit map, mapa de espaço livre
            //-------------------------------------- // Bit map, mapa de espaço 
    // Escreve state do arquivo
            memmove(recvBuffer, recvBuffer + 3, strlen(recvBuffer)); // Escreve state do arquivo

            chdir(rec
            printf("Entra no diretorio %s\n", recvBu
            pthread_mutex_unlock(&cadeado);
            
            //--------------------------------------
        

        //mostrar conteudo do diretorio
        if (strcmp(recvBuffer, "ls") == 0){

            char *buffer = ls();

            send(*conexao, buffer, strlen(buffer), 0);

        }

        //criar arquivo
        if (strncmp(recvBuffer, "touch ", 6) == 0){
            pthread_mutex_lock(&cadeado);

            status = touch(recvBuffer);

            if(!status) printf("Arquivo criado com sucesso\n");
            else printf ("Erro ao criar arquivo\n");

            pthread_mutex_unlock(&cadeado);
        }

        //remover arquivo
        if ((strncmp(recvBuffer, "rm ", 3) == 0) && (strncmp(recvBuffer, "rm -r ", 6) != 0) ){
            pthread_mutex_lock(&cadeado);
            
            //status = rm(recvBuffer);

            if(!status) printf("Arquivo removido com sucesso\n");
            else printf("Erro ao remover arquivo\n");

            pthread_mutex_unlock(&cadeado);
        }

        //escrever um sequencia de caracteres em um arquivo
        if (strncmp(recvBuffer, "echo ", 5) == 0){
            pthread_mutex_lock(&cadeado);

            //status = echo(recvBuffer);

            if(!status) printf("Caracteres inseridos com sucesso\n");
            else printf("Erro ao inserir caracteres\n");

            pthread_mutex_unlock(&cadeado);
        }

        //mostrar conteudo do arquivo
        if (strncmp(recvBuffer, "cat ", 4) == 0){
            pthread_mutex_lock(&cadeado);

            //sendBuffer = cat(recvBuffer);
            send(*conexao, sendBuffer, strlen(sendBuffer), 0);
            
            pthread_mutex_unlock(&cadeado);
        }

    }
    printf("Conexao fechada\n");
}


int find_inode(){

    // Cria inode temporario
    inode temp;
    int i;

    // Busca o primeiro inode livre
    for(i = 0; i < NBLOCKS; i++){
        // Vai pulando de inode em inode
        fseek(file, (map_size + i * sizeof(inode)), SEEK_SET);
        // Le o inode correspondente
        fread(&temp, sizeof(inode), 1, file);
        // Verifica o state do inode
        if(temp.state == 0)
            break;
    }

    // Se o inode estiver livre, altera o state para ocupado
    temp.state = 1;
    fseek(file, (map_size + i * sizeof(inode)), SEEK_SET);
    fwrite(&temp, sizeof(inode), 1, file);

    return i;
}


int mkdir_(char* recvBuffer){

    inode temp;
    int inode_pos, status;
    bool state = 1;
    char  name;

    // Posicao do inode livre
    inode_pos = find_inode();
    
    // Pula para o inode livre
    fseek(file, (map_size + inode_pos * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);
     // Retira o "mkdir " do comando
    name = strtok(recvBuffer," ");
    
 // Bit map, mapa de espaço livre
    // Insere os dados no i
    strcpy(temp.type, "Dir");  // Escreve state do arquivo
    temp.block = find_block();

    // Cria um diorio temp
    dir temp_;
    temp_.atual = inode_pos;
    temp_.ant = dir_atual;

    // Pula para o bloco livre encontrado e insere o diretorio no arquivo
    fseek(file, (map_size + inode_tam + temp.block * BLOCK_SIZE), SEEK_SET);    
    fwrite(&temp_, sizeof(temp_), 1, file);   
    
    status = 0;
    
    return status;
}

int touch(char* recvBuffer){

    int bloco_livre, bloco_dir;
    inode temp;
    dir atual;
    char  name;

    // Procura o primeiro bloco livre
    bloco_livre = find_block();

    // Pula para o inode do diretorio atual e le o mesmo
    fseek(file, (map_size + dir_atual * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);

    // Posicao do bloco do diretorio atual
    bloco_dir = temp.block;

    // Pula para a posicao do bloco do diretorio atual e le o mesmo
    fseek(file, (map_size + inode_tam + bloco_dir * BLOCK_SIZE), SEEK_SET);
    fread(&atual, BLOCK_SIZE, 1, file);

    name = strtok(recvBuffer," ");
    name = strtok(NULL, " ");

    // Procura uma posicao livre na lista de dados do diretorio atual
    int i;
    for(i = 0; i < MAX_ARQ; i++){
        if(atual.arq[i].inode != 0)
            break;
    }

    // Insere os dados do arquivo criado na lista de dados do diretorio atual
    if(i != MAX_ARQ){
        strcpy(atual.arq[i] name, name);
        atual.arq[i].inode = bloco_livre;
    }
    else{
        printf("Diretorio cheio\n");
        exit(0);
    }

    // Pula para o bloco do diretorio atual e atualiza o mesmo
    fseek(file, (map_size + inode_tam + bloco_dir * BLOCK_SIZE), SEEK_SET);
    fwrite(&atual, sizeof(dir), 1, file);

    return 0;
    
}

char* ls(){
    
    inode temp;
    int bloco;
    dir atual;
    char *lista = (char*) malloc(sizeof(char));

    // Pula para o inode do diretorio atual e le o mesmo 
    fseek(file, (map_size + dir_atual * sizeof(inode)), SEEK_SET);
    fread(&temp, sizeof(inode), 1, file);

    bloco = temp.block;

    // Pula para o bloco do diretorio atual e le o mesmo
    fseek(file, (map_size + inode_tam + bloco * BLOCK_SIZE), SEEK_SET);
    fread(&atual, BLOCK_SIZE, 1, file);
        
    strcat(lista, "\t.\t..");

    // Le a lista de dados que esta no bloco do diretorio atual
    for(int i = 0; i < MAX_ARQ; i++){
        if(atual.arq[i] name != NULL){
            strcat(lista, "\t");
            strcat(lista, atual.arq[i] name);
        }
        else break;
    }

    // Insere o caracter de finalizacao de string no final da lista
    lista[strlen(lista) - 2] = '\n';
    lista[strlen(lista) - 1] = '\0';

    return lista;

}

int main(){

    int socketR = create_socket();

    printf("Socket criado\n");
    
    // Inicializando sistema de arquivo
    int initialize = fylesystem();

    printf("Sistema de Arquivo criado com sucesso\n");

    while(1){
        // Aceita conexoes
        int *connfd = calloc(sizeof(int), 1);

        printf("\nEsperando conexao\n");        
        *connfd = accept(socketR, (struct sockaddr*)NULL, NULL);

        if(*connfd == -1){
            printf("Erro ao aceitar a conexao\n");
            exit(-1);
        }
        printf("Conexao criada\n");

        pthread_t tid;
        pthread_create(&tid, NULL, comando, connfd);
        printf("Thread criada.\n");

    }

    return 0;
}