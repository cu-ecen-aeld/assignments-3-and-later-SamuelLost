#!/bin/sh
# 9) Escreva um script de shell finder-app/finder.sh conforme descrito abaixo:

# Aceita os seguintes argumentos de tempo de execução: o primeiro argumento é um caminho para um diretório no sistema de arquivos, chamado abaixo de filesdir; 
# o segundo argumento é uma cadeia de texto que será pesquisada nesses arquivos, chamada abaixo de searchstr

# Sai com valor de retorno 1, erro e imprime instruções se algum dos parâmetros acima não tiver sido especificado

# Sai com valor de retorno 1 e imprime instruções se filesdir não representar um diretório no sistema de arquivos

# Imprime uma mensagem "The number of files are X and the number of matching lines are Y" (O número de arquivos é X e o número de linhas correspondentes é Y), 
# em que X é o número de arquivos no diretório e em todos os subdiretórios e Y é o número de linhas correspondentes encontradas nos respectivos arquivos, 
# em que uma linha correspondente se refere a uma linha que contém searchstr (e também pode conter conteúdo adicional).

# Exemplo de uso:
# finder.sh /tmp/aesd/assignment1 linux


filesdir=$1
searchstr=$2

usage() {
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1
}

if [ -z $filesdir ] || [ -z $searchstr ]; then
    usage
    exit 1
fi

if [ ! -d $filesdir ]; then
    echo "Error: $filesdir is not a directory"
    usage
    exit 1
fi

numfiles=$(find $filesdir -type f | wc -l)
numlines=$(grep -r $searchstr $filesdir | wc -l)

echo "The number of files are $numfiles and the number of matching lines are $numlines"