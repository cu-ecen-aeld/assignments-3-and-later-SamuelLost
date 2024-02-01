# 10) Escreva um script de shell finder-app/writer.sh conforme descrito abaixo

# Aceita os seguintes argumentos: o primeiro argumento é um caminho completo para um arquivo (incluindo o nome do arquivo) 
# no sistema de arquivos, chamado abaixo de writefile; o segundo argumento é uma string de texto que será gravada nesse arquivo, 
# chamada abaixo de writestr

# Sai com o valor 1 de erro e imprime instruções se algum dos argumentos acima não tiver sido especificado

# Cria um novo arquivo com nome e caminho writefile com conteúdo writestr, sobrescrevendo qualquer arquivo existente e criando o 
# caminho se ele não existir. Sai com o valor 1 e com a instrução de impressão de erro se o arquivo não puder ser criado.

# Exemplo de uso:
# writer.sh /tmp/aesd/assignment1/sample.txt ios

writefile=$1
writestr=$2

usage() {
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
}

if [ -z $writefile ] || [ -z $writestr ]; then
    usage
    exit 1
fi

mkdir -p $(dirname $writefile)

echo $writestr > $writefile
