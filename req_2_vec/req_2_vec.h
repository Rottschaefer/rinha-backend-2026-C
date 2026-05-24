#ifndef VEC_SEARCH_H
#define VEC_SEARCH_H

#include <stdio.h>

// Sub-estrutura para last_transaction
typedef struct {
    char timestamp[32];      // String ISO 8601 (ex: "2026-05-12T10:00:00Z")
    float km_from_current;
} LastTransaction;

// Sub-estrutura para transaction
typedef struct {
    float amount;
    int installments;
    char requested_at[32];   // String ISO 8601
} TransactionInfo;

// Sub-estrutura para customer
typedef struct {
    float avg_amount;
    int tx_count_24h;
    char** known_merchants;  // Array de strings dinâmico
    int known_merchants_count; // Quantidade de comerciantes no array
} CustomerInfo;

// Sub-estrutura para merchant
typedef struct {
    char id[64];             // Identificador do comerciante
    char mcc[16];            // Código MCC
    float avg_amount;
} MerchantInfo;

// Sub-estrutura para terminal
typedef struct {
    int is_online;
    int card_present;
    float km_from_home;
} TerminalInfo;

// Estrutura principal da Requisição
typedef struct {
    char id[64];             // Identificador da transação
    
    TransactionInfo transaction;
    CustomerInfo customer;
    MerchantInfo merchant;
    TerminalInfo terminal;
    
    int has_last_transaction; // Flag que indica se last_transaction não é null
    LastTransaction last_transaction;
} TransactionRequest;

// Estrutura para os dados de normalização
typedef struct {
    float max_amount;
    int max_installments;
    float amount_vs_avg_ratio;
    int max_minutes;
    float max_km;
    int max_tx_count_24h;
    float max_merchant_avg_amount;
} Normalization;


Normalization get_normalization_data(void);
float get_mcc_risk(const char* mcc);
void vectorize_request(TransactionRequest* req, float* out_vector);

#endif