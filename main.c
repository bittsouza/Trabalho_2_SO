/*
01/10/2018

Trabalho 1 de Arquitetura de SO
Engenharia da Computação
UFSC - Campus Araranguá
Desenvolvido por:
Gabriel Bittencourt de Souza (matrículo: e Marvin Gasque Teófilo da Silva (matrícula: 15103101)

Prof. Martín Vigil

Enunciado do Trabalho: 
Desenvolver um sistema de arquivos distribuídos que simula padrão EXT3. 
O sistema deve ser desenvolvido em C ou C++ utilizando o compilador GNU GCC e chamadas de sistemas do padrão POSIX. 
O sistema deve permitir que arquivos locais sejam acessados por usuários remotos simultaneamente. 
As operações permitidas pelo sistema devem incluir:
	
	criar (sub)diretório (mkdir)
	remover (sub)diretório (rm -r)
	entrar em (sub)diretório (cd)
	mostrar conteúdo do diretório (ls)
	criar arquivo (touch)
	remover arquivo (rm)
	escrever um sequência de caracteres em um arquivo (echo) - echo "texto" > 'arquivo'
	mostrar conteúdo do arquivo (cat)

Etapa 1
Desenvolver a estrutura de acesso do servidor de arquivos. Ele deverá será acessado via socket TCP.
Cada conexão deverá ser gerida por uma thread. Condições de corrida deverão ser tratadas por meio de semáforos ou mutexes.
Nesta etapa você não precisa implementar as operações sobre arquivos listadas acima.
Ao invés disso, use as operações diretamente do sistema de arquivos do seu sistema operacional.
Recomenda-se que o servidor imprima mensagens na tela para demonstrar o funcionamento ao professor.

Observações:
Não é necessário autenticação dos usuários.
Não é necessário criar um aplicativo cliente. Você pode usar o aplicativo netcat disponível para Linux e Windows.
*/

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


	pthread_mutex_t mutex; // precisa ser global!

	char* listDir();

    int create_socket();
    void* comando(void* connfd);
    int initialize_fs();
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


struct inode_{
        char name[5];   
        char tipo[3];   
        int bloco;      
        bool estado;    
    };
    typedef struct inode_ inode;

    // Dados
    struct dado_{   
        char name[5];
        int inode;
    };  
    typedef struct dado_ dado;

    // Diretorio
    struct dir_{
        int atual;
        int ant;
        dado arq[MAX_ARQ];
    };
    typedef struct dir_ dir;

// Variáveis globais
    // Mutex
    pthread_mutex_t cadeado;
    
    // Tamanho das particoes
    int mapa_tam = sizeof(bool) * NBLOCKS;
    int inode_tam = sizeof(inode) * NBLOCKS;
    int blocos_tam = BLOCK_SIZE * NBLOCKS;
    int dir_atual;

    FILE *file;

/*void *ls(void *threadid, char msg){
   	int *connfd = (int *)threadid;
   	char sendBuff[1025];

   	while (strcmp("exit\n", sendBuff) != 0){
	   	int recebido = recv(*connfd, sendBuff, 1025, 0);
		sendBuff[recebido] = '\0';
	   
	   	if ((strncmp("mkdir ", sendBuff, 6)) == 0){
			printf("\nESTOU NO MKDIR\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("rm -r ", sendBuff, 6)) == 0){
			printf("\nESTOU NO RM -R\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);		
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("cd ", sendBuff, 3)) == 0){
			
				DIR *d;
			    struct dirent *dir;
			    d = opendir(".");

			    if (d)
			    {
			        while ((strncmp("cd ..",sendBuff, 5))== 0)
			        {
			         	
			         	 while ((dir = readdir(d)) != NULL) {
            			printf ("[%s]\n", dir->d_name);
		        		}

			        }
			        closedir(d);
			    	}

			printf("\nESTOU NO CD\n");
			pthread_mutex_lock(&mutex);

			opendir("old");
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("ls", sendBuff, 2)) == 0){
			printf("\nESTOU NO LS\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			  	
			  	char* ls = listDir();
				write(*connfd, ls, 1024);
   				free(ls);
 
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("touch ", sendBuff, 6)) == 0){
			printf("\nESTOU NO TOUCH\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("rm ", sendBuff, 3)) == 0){
			printf("\nESTOU NO RM\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("echo ", sendBuff, 5)) == 0){			
			printf("\nESTOU NO ECHO\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			pthread_mutex_unlock(&mutex);
		}

		if ((strncmp("cat ", sendBuff, 4)) == 0){
			printf("\nESTOU NO CAT\n");
			pthread_mutex_lock(&mutex);
			system(sendBuff);
			pthread_mutex_unlock(&mutex);
		}
	}
}

*/

char* listDir(){
	char *retorno = calloc(2048,1);
	int offset = 0;

	DIR *d;
    struct dirent *dir;
    d = opendir(".");

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            offset = offset + sprintf(retorno + offset, "%s ", dir->d_name);
        }
        closedir(d);
    }

    return retorno;
}

void* comando(void* connfd){
    int* conexao = (int*)connfd;
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
            pthread_mutex_lock(&mutex);

            status = mkdir_(recvBuffer);

            if(!status) printf("Pasta criada com sucesso\n");
            else printf ("Erro ao criar pasta\n");
        
            pthread_mutex_unlock(&mutex);
        }

        //remover (sub)diretorio
        if (strncmp(recvBuffer, "rm -r ", 6) == 0){
            pthread_mutex_lock(&mutex);

            //status = rm_r(recvBuffer);
            
            if(!status) printf("Pasta excluida com sucesso\n");
            else printf ("Erro ao excluir pasta\n");

            pthread_mutex_unlock(&mutex);
        }

        //entrar em (sub)diretorio
        if (strncmp(recvBuffer, "cd ", 3) == 0){
            
            //--------------------------------------

            pthread_mutex_lock(&mutex);
            memmove(recvBuffer, recvBuffer + 3, strlen(recvBuffer));

            chdir(recvBuffer);
            printf("Entrando no diretorio %s\n", recvBuffer);
            pthread_mutex_unlock(&mutex);
            
  
        }

        //mostrar conteudo do diretorio
        if (strcmp(recvBuffer, "ls") == 0){

            char *buffer = ls();

            send(*conexao, buffer, strlen(buffer), 0);

        }

        //criar arquivo
        if (strncmp(recvBuffer, "touch ", 6) == 0){
            pthread_mutex_lock(&mutex);

            status = touch(recvBuffer);

            if(!status) printf("Arquivo criado com sucesso\n");
            else printf ("Erro ao criar arquivo\n");

            pthread_mutex_unlock(&mutex);
        }

        //remover arquivo
        if ((strncmp(recvBuffer, "rm ", 3) == 0) && (strncmp(recvBuffer, "rm -r ", 6) != 0) ){
            pthread_mutex_lock(&mutex);
            
            //status = rm(recvBuffer);

            if(!status) printf("Arquivo removido com sucesso\n");
            else printf("Erro ao remover arquivo\n");

            pthread_mutex_unlock(&mutex);
        }

        //escrever um sequencia de caracteres em um arquivo
        if (strncmp(recvBuffer, "echo ", 5) == 0){
            pthread_mutex_lock(&mutex);

            //status = echo(recvBuffer);

            if(!status) printf("Caracteres inseridos com sucesso\n");
            else printf("Erro ao inserir caracteres\n");

            pthread_mutex_unlock(&mutex);
        }

        //mostrar conteudo do arquivo
        if (strncmp(recvBuffer, "cat ", 4) == 0){
            pthread_mutex_lock(&mutex);

            //sendBuffer = cat(recvBuffer);
            send(*conexao, sendBuffer, strlen(sendBuffer), 0);
            
            pthread_mutex_unlock(&mutex);
        }

    }
    printf("Conexao fechada\n");
}


int main(int argc, char **argv){

	int listenfd = 0;
	struct sockaddr_in serv_addr;
	char sendBuff[1025];
	char msg[1025] = "oiebit";

	listenfd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&serv_addr, '0', sizeof(serv_addr));
		
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5001);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	int startfs = start_fs(); //inicia sistema de arquivos
}

	while(1){


		int *connfd = calloc(sizeof(int),1);
		pthread_t thread;

		*connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
			
		printf("Socket criado: %d\n", *connfd);
		pthread_create(&thread, NULL, initialize_fs, connfd);		
	}
	return 0;
}
