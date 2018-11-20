# Trabalho_1_SO


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
	escrever um sequência de caracteres em um arquivo (echo)
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
