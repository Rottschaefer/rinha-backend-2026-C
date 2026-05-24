# Rinha Backend 2026 (C Edition) 🚀

Esta API foi desenvolvida para a Rinha de Backend, focando em performance extrema, baixo consumo de memória e baixa latência.

## Decisões de Projeto

### Servidor Web: C + facil.io
Decidimos por usar C, devido ao controle fino e a performance que a linguagem fornece, em conjunto com o **facil.io**. A biblioteca tem arquitetura orientada a eventos, reduzindo overhead de criação de threads(o que não se faz necessário na rinha, já que tudo será executado em uma CPU) e possui alocação de memória eficiente.

### Formato de Dados: Binário vs JSON
O dataset original é fornecido em JSON comprimido (`.gz`). No entanto, processar JSON em tempo de execução é proibitivo para alta performance devido ao maior uso de memória por ser em formato texto(comparado ao binário), mas principalmente pelo overhead de parsing.

**Solução:** Convertemos o JSON em um arquivo binário customizado (`.bin`) durante o build. Isso permite que a API carregue somente os dados "crus" necessários(sem nome de chaves, strings da estrutura do objeto, etc.), exatamente como serão usados na memória.

### Alinhamento de Memória e Cache Lines (64 Bytes)
Cada registro tem 57 bytes + 7 bytes de padding, totalizando 64 bytes. Essa decisão vem do fato do processador ler a RAM em blocos de **64 bytes** (Cache Lines), então esse alinhamento vai gerar um ganho de performance (pelo menos, teoricamente).

```c
typedef struct __attribute__((packed)) {
    float coords[14];   // 56 bytes (14 * 4)
    unsigned char label;       // 1 byte  (fraud/legit)
    unsigned char padding[7];  // 7 bytes de recheio manual
} Record; // Total: Exatos 64 bytes
```

## Como Executar

Antes de iniciar a API, é necessário gerar o arquivo de dados em formato binário a partir do JSON utilizando o script Python criado para isso.

**1. Gerar o arquivo binário:**
```bash
python3 json_2_bin.py resources/references.json.gz resources/references.bin
```

**2. Compilar a biblioteca facil.io:**
Para otimizar o tempo de compilação da aplicação, podemos pré-compilar o `facil.io` em uma biblioteca estática:

```bash
gcc -c -O3 facil.io/src/*.c -I./facil.io/include
ar rcs libfacil.a *.o
rm *.o
```

**3. Compilar e Executar a API:**
Compile o projeto (`main.c` e `vec_search.c`) apontando para as bibliotecas e executando em seguida:

```bash
gcc -O3 -o api_main main.c vec_search/vec_search.c \
  ./usearch/usearch_linux_amd64_2.25.2.so \
  -I. \
  -I./facil.io/include \
  -L. -lfacil \
  -Wl,-rpath,./usearch \
  -pthread -lm

./api_main
```
