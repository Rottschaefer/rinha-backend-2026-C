FROM ubuntu:24.04 AS builder

# Instala as dependências mínimas para build
RUN apt-get update && apt-get install -y gcc make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Compila o projeto com otimização -O3 para a rinha
RUN gcc -O3 -o api_main main.c facil.io/src/*.c -I ./facil.io/include -pthread -lm

# Imagem final extremamente enxuta
FROM ubuntu:24.04
WORKDIR /app
COPY --from=builder /app/api_main .

EXPOSE 9999
CMD ["./api_main"]
