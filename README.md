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

Antes de iniciar a API, é necessário gerar o arquivo de index para uso do USearch e um arquivo de labels com o label de cada vetor do dataset. Execute o arquivo python para isso:

**1. Preparar o Ambiente Python e Dependências:**
O script de indexação necessita de algumas bibliotecas (como `numpy` e `usearch`). É recomendado executá-lo dentro de um ambiente virtual (venv):

```bash
python3 -m venv venv
source venv/bin/activate  # No Windows: venv\Scripts\activate
pip install numpy usearch
```

**2. Gerar o arquivo binário:**
```bash
python3 create_index.py resources/references.json.gz resources/labels
```

**3. Compilar a biblioteca facil.io:**
Para otimizar o tempo de compilação da aplicação, podemos pré-compilar o `facil.io` em uma biblioteca estática:

```bash
gcc -c -O3 facil.io/src/*.c -I./facil.io/include
ar rcs libfacil.a *.o
rm *.o
```

**4. Compilar e Executar a API:**
Compile o projeto (`main.c`, `req_2_vec.c` e `usearch`) apontando para as bibliotecas e executando em seguida:

```bash
gcc -O3 -o api_main main.c req_2_vec/req_2_vec.c kdtree/kdtree.c -DUSE_LIST_NODE_ALLOCATOR -lpthread \
  ./usearch/libusearch_c.so \
  -I. \
  -I./facil.io/include \
  -L. -lfacil \
  -Wl,-rpath,./usearch \
  -pthread -lm

./api_main
```
