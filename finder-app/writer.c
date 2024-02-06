// 3. Escreva um aplicativo "writer" em C(finder-app/writer.c)  que possa ser usado como alternativa ao script de teste "writer.sh" criado na atribuição 1 
// e usando o File IO conforme descrito no capítulo 2 do LSP. Consulte os requisitos da Atribuição 1 para o script de teste writer.sh e essas instruções adicionais:
// Uma diferença em relação às instruções do write.sh no Exercício 1: O senhor não precisa fazer com que o utilitário "writer" crie diretórios que não existam.  
// O senhor pode presumir que o diretório foi criado pelo chamador.

// Configure o registro em log do syslog para o seu utilitário usando o recurso LOG_USER.
// Use o recurso syslog para gravar uma mensagem "Writing <string> to <file>", em que <string> é a cadeia de texto gravada no arquivo (segundo argumento) 
// e <file> é o arquivo criado pelo script. Isso deve ser gravado com o nível LOG_DEBUG.
// Use o recurso syslog para registrar qualquer erro inesperado com o nível LOG_ERR.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char *argv[]) {

    openlog("Writer", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DEBUG);

    if (argc != 3) {
        syslog(LOG_ERR, "Error: Invalid number of arguments");
        exit(1);
    }

    if (strlen(argv[1]) == 0 || strlen(argv[2]) == 0) {
        syslog(LOG_ERR, "Error: Empty arguments");
        exit(1);
    }

    FILE *file = fopen(argv[1], "w+");
    char *string = argv[2];

    if (file == NULL) {
        syslog(LOG_ERR, "Error: Could not open file");
        exit(1);
    }

    fwrite(string, sizeof(char), strlen(string), file);
    syslog(LOG_DEBUG, "Writing %s to %s", string, argv[1]);
    fclose(file);
    closelog();
}