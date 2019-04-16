#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>


/* Tamanho máximo do bloco(em Bytes) à ser lido e escrito
 * por read() e write() no parâmetro "count" */
#define MAX_SZ_BLOCK 5000

/* Tempo em segundo(s) de parâmetro para função alarm() */
#define TIME_ALARM 1

/* Enviar SIGALRM a cada "TIME_ALARM" segundo(s),
 * imprimindo mensagem de status a cada chamada
 * Complexidade Θ(1) */
void alarmStatus(){
    printf("Cópia em andamento...\n");
    alarm(TIME_ALARM);
}

/* Abre um arquivo de nome "fileName com as permissões "mode"
 * Retorna retorno de open(), >2 se Ok;
 * Caso de erro, imprime mensagem de erro e retorna -1.
 * Complexidade Θ(1) */
int openFile(char *fileName, mode_t mode){
    int fd = open(fileName, mode);
    
    if(fd == -1)
        printf("copyit: Não foi possível abrir o arquivo\"%s\" - %s\n", fileName, strerror(errno));
    
    return fd;
}

/* Cria um arquivo de nome "fileName com as permissões "mode"
 * Retorna retorno de creat(), >2 se Ok;
 * Caso de erro, imprime mensagem de erro e retorna -1.
 * Complexidade Θ(1) */
int creatFile(char *fileName, mode_t mode){
    int fd = creat(fileName, mode | S_IRWXU);
    
    if(fd == -1)
        printf("copyit: Não foi possível abrir o arquivo\"%s\" - %s\n", fileName, strerror(errno));
    
    return fd;
}

/* Captura o tamanho do arquivo de nome "*filename"
 * Retorna o tamanho do arquivo ou -1 caso erro.
 * Complexidade Θ(1) */
long int lengthFile(char *filename){
    /* Usa função stat() para capturar as informações do
     * arquivo de nome *filename*/
     
    struct stat fileStat; // Estrutura onde os dados do arquivo serão armazenados
 
    stat(filename, &fileStat);

    return (long int)fileStat.st_size; // st_size armazena tamanho do arquivo
}

/* Calcula os blocks necessário para "algoritmo de cópia".
 * "numBytes é o tamanho do arquivo passado (restantande de bytes);
 * "*qntBlocks" o apontador para o qntBlocks(quantidade de blocos) usado no alg. de cópia;
 * "*sizeBlock" o apontador para o sizeBlock(tamanho do bloco) usado no alg. de cópia;
 * Retorna o número de bytes que restante do arquivo;
 * Complexidade O(MAX_SZ_BLOCK/10) */
unsigned long int calcBlocks(unsigned long int numBytes, unsigned long int *qntBlocks, unsigned long int *sizeBlock){
    /* Variáveis de manutanção do alg. de cálculo de blocks */
    unsigned long int bytesLeft;    // Qnt de bytes de resto. Tamanho do arquivo corrente
    float divisor;                  // Divisor do cálculo de blocos
    float maxSizeBlockFloat;        // Aux. nos cálculos com ponto fluentante e MAX_SZ_BLOCK
    
    /* Obs: Uso conversões explícitas -"cast" de entre valores
     * do tipo "float" e "int", e vice-versa, para adequar aos cálculos e comparações*/
    
    /* Inicializa variváveis de manutenção */
    divisor = 1.0;                  // Menor valor válido
    maxSizeBlockFloat = (float)MAX_SZ_BLOCK; // MAX_SZ_BLOCK convertido para ponto flutuante
    
    /* Se a qnt de bytes passada for menor 1, encerra o cálculo e
     * retorna "numBytes", que neste estágio é =0*/
    if(numBytes < (unsigned long int)divisor)
        return numBytes;
    
    /* Incrementa em x10 o divisor do cálculo da qnt e tamanho dos blocks enquanto:
     * o divisor for menor igual(<=) que MAX_SZ_BLOCK; e
     * "numBytes" for menor que a divisão de MAX_SZ_BLOCK por "divisor" */
    while((MAX_SZ_BLOCK > ((unsigned long int)divisor)) && (numBytes <= (unsigned long int)(maxSizeBlockFloat/divisor)))
        divisor = divisor *10;     // Incrementa divisor em x10.
    
    /* Atualiza *qntBlocks, *sizeBlock e valor de retorno, bytesLeft */
    
    /* MAX_SZ_BLOCK ainda é maior igual o divisor. Nesse caso "numBytes" é menor que o
     * menor tamanho de bloco possível, que é ((MAX_SZ_BLOCK*10)/divisor) */
    if(MAX_SZ_BLOCK >= (unsigned long int)divisor){
        // Atualiza *sizeBlocks com novo, e mais adequado, tamanho de bloco
        *sizeBlock =  maxSizeBlockFloat/divisor; 
        // Atualiza *qntBlocks com a qnt de blocos correspondente ao novo tamanho de block
        *qntBlocks = (unsigned long int)(numBytes / *sizeBlock);
        // Calcula a qnt de bytes de resto, byte que sobraram da divisão com novo tamanho de bloco
        bytesLeft =  (numBytes % *sizeBlock);
    }
    /* Se o divisor ultrapassar MAX_SZ_BLOCK. Nesse caso "numBytes" é menor que o
     * menor tamanho de bloco possível, que é ((MAX_SZ_BLOCK*10)/divisor) */
    else{
        // Atualiza *sizeBlocks com a qnt corrente de bytes do arquivo passada nos parâmetros
        *sizeBlock = numBytes;
        // Os bytes restante formam apenas um bloco
        *qntBlocks = 1;
        // Não há bytes sobrando (resto do cálculo)
        bytesLeft =  0;
    }

    return bytesLeft;
}

/* FUNÇÃO PRINCIAL - copyit
 * Complexidade Θ(QUAL É?) */
int main(int argc, char *argv[]){
    /*Verifica se a chamada de "copyit" estão com os parâmetros corretos,
     * "copyit arquivoOrigem arquivoDestino". Caso não, encerra o programa */
    if(argc != 3){
        printf("copyit: Número incorreto de argumentos\n");
        printf("\tuse: \"copyit ArquivoOrigem ArquivoDestino\"\n");
        exit(1);
    }
    
    /* Nomes dos arquivos captados em argv[] */
    char *fileNameIn = argv[1];    // Origem/Original
    char *fileNameOut = argv[2];   // Destino/Cópia
    
    /* Descritores de arquivos */
    int fdIn;       // Origem/Original
    int fdOut;      // Destino/Cópia
    
    /* Váriáveis de manutenção do algoritmo de cópia*/
    unsigned long int lenFileIn;                  // Tam. do arquivoOrigem em bytes
    unsigned long int sizeBlock;                  // Tam. do bloco corrente para ser lidos/escritos com read()/write() no parâmetro "count" 
    unsigned long int qntBlocks;                  // Qnt de blocos a serem lidos/escritos com read()/write()
    unsigned long int cntRWBlocks;                // Contador de blocos lidos/escritos com read()/write()
    unsigned long int bytesLeft;                  // Qnt de bytes de resto. Auxilia cálculo de "sizeBlock" e "qntBlocks"
    unsigned long int cntBytes;                   // Qnt de bytes lidos do arquivoOrigem para arquivoDestino

    /* Conecta o alarme de SIGALRM à alarmStatus() */
    signal(SIGALRM, alarmStatus);
    alarm(TIME_ALARM);        // Inicia a despertador de entrega de sinal

    /* Abre arquivo de nome "fileNameIn" no modo "apenas leitura" e
     * inicia "fdIn" com descritor de arquivo que, se inválido, encerra o programa */
    if((fdIn = openFile(fileNameIn, O_RDONLY)) <0)
        exit(1);
    /* Cria arquivo de nome "fileNameOut" no modo "apenas leitura" e
     * inicia "fdOut" com descritor de arquivo que, se inválido, encerra o programa */
    else if((fdOut = creatFile(fileNameOut, O_WRONLY)) <0)
        exit(1);
    
    /* Inicializa váriáveis de manunetenção do algoritmo de cópia */
    if((lenFileIn = lengthFile(fileNameIn)) < 0){   // Captura o tamanho do arquivo
        printf("copyit: Não foi possível ler o arquivo \"%s\"", fileNameIn);
        exit(1);
    }
    bytesLeft = lenFileIn;                  // Tamanho do arquivoOrigem
    qntBlocks = sizeBlock = cntBytes = 0;
    
    /* Algoritmo de cópia */
    while(bytesLeft > 0){
        
        /* Atualiza bytesLeft, qntBlocks e sizeBlocks de acordo com o cálculo de blocos */
        bytesLeft = calcBlocks(bytesLeft, &qntBlocks, &sizeBlock);
        
        /* Buffer de tamanho que pode variar a cada iteração. Parâmetro de read()/write()*/
        char buf[sizeBlock];
    
        cntRWBlocks =0;     // Reseta contador de bytes lidos/escritos
        
        while(cntRWBlocks < qntBlocks){
            /* Lê/Copia para "buf" "sizeBlock" bytes do arquivoOrigem. Em caso de
             * falha (read() =-1), exibe mensagem de erro e encerra o programa*/
            if((read(fdIn, &buf, sizeBlock)) == -1){
                printf("copyit: Erro ao copiar arquivo \"%s\" - %s\n", fileNameIn, strerror(errno));
                exit(1);
            }
            /* Escreve de "buf" "sizeBlock" bytes no arquivoDestino. Em caso de
             * falha (write() =-1), exibe mensagem de erro e encerra o programa*/
            if(write(fdOut, &buf, sizeBlock) == -1){
                printf("copyit: Erro ao escrever no arquivo \"%s\" - %s\n", fileNameOut, strerror(errno));
                exit(1);
            }
            
            cntRWBlocks++;  // Incrementa o contador de blocos lidos/escritos
        }
        
        /* Atualiza cntBytes com número de bytes lidos/escritos na iteração corrente */ 
        cntBytes += (sizeBlock * qntBlocks);
    }
    /* Fecha (retira da tabela) fdIn e fdOut */
    close(fdIn);
    close(fdOut);

    /* Compara o "cntBytes" lidos/escritos com tamanho do
     * arquivo de entrada e exibe mensagem adequada */
    if(cntBytes == lenFileIn)
        printf("\ncopyit: Foram copiados %lu bytes do arquivo \"%s\" para o arquivo \"%s\"\n", cntBytes, fileNameIn, fileNameOut);
    else
        printf("copyit: Erro não previsto ao copiar arquivo \"%s\" - \"%s\"\n", fileNameIn, fileNameOut);
    
    exit(1);
}
