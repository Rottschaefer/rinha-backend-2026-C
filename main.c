#include <http.h>
#include <stdio.h>
#include <string.h>
#include "vec_search/vec_search.h"
#include <time.h>

#define TRUE 1
#define FALSE 0
#define K 5

typedef struct __attribute__((packed)) {
float coords[14];
unsigned char label;
unsigned char padding[7];
} Record;

// Arquivo global aberto na inicialização e acessível aos requests
FILE* db_file = NULL;

void parse_json(TransactionRequest* transaction, char* body){
    //Optamos por fazer um parse simples json->struct usando uma combinação de strstr com sscanf que leva em conta a estrutura da requisição. Não funcionaria para outros modelos
    char* ptr = body;

    // ID
    ptr = strstr(ptr, "\"id\"");
    if (ptr) sscanf(ptr, "\"id\": \"%63[^\"]\"", transaction->id);

    // Transaction
    ptr = strstr(body, "\"transaction\"");
    if (ptr) {
        char* amount = strstr(ptr, "\"amount\"");
        if (amount) sscanf(amount, "\"amount\": %f", &transaction->transaction.amount);

        char* inst = strstr(ptr, "\"installments\"");
        if (inst) sscanf(inst, "\"installments\": %d", &transaction->transaction.installments);

        char* req_at = strstr(ptr, "\"requested_at\"");
        if (req_at) sscanf(req_at, "\"requested_at\": \"%31[^\"]\"", transaction->transaction.requested_at);
    }

    // Customer
    ptr = strstr(body, "\"customer\"");
    if (ptr) {
        char* avg = strstr(ptr, "\"avg_amount\"");
        if (avg) sscanf(avg, "\"avg_amount\": %f", &transaction->customer.avg_amount);

        char* tx_count = strstr(ptr, "\"tx_count_24h\"");
        if (tx_count) sscanf(tx_count, "\"tx_count_24h\": %d", &transaction->customer.tx_count_24h);

        char* km = strstr(ptr, "\"known_merchants\"");
        if (km) {
            // Arrays estáticos para armazenar strings de merchants alocação zero 
            //Se formos usar mais de uma thread, esse static pode ser um problema
            static char merch_buffer[20][64];
            static char* merch_ptrs[20];
            transaction->customer.known_merchants = merch_ptrs;
            transaction->customer.known_merchants_count = 0;
            
            char *array_start = strchr(km, '[');
            char *array_end = array_start ? strchr(array_start, ']') : NULL;
            
            if (array_start && array_end) {
                char *merch_ptr = array_start;
                while (merch_ptr < array_end && transaction->customer.known_merchants_count < 20) {
                    merch_ptr = strchr(merch_ptr, '"');
                    if (!merch_ptr || merch_ptr > array_end) break;
                    merch_ptr++; // pula aspas
                    
                    int idx = transaction->customer.known_merchants_count;
                    sscanf(merch_ptr, "%63[^\"]", merch_buffer[idx]);
                    merch_ptrs[idx] = merch_buffer[idx];
                    transaction->customer.known_merchants_count++;
                    
                    merch_ptr = strchr(merch_ptr, '"');
                    if (merch_ptr) merch_ptr++;
                }
            }
        }
    }

    // Merchant
    ptr = strstr(body, "\"merchant\"");
    if (ptr) {
        char* m_id = strstr(ptr, "\"id\"");
        if (m_id) sscanf(m_id, "\"id\": \"%63[^\"]\"", transaction->merchant.id);

        char* mcc = strstr(ptr, "\"mcc\"");
        if (mcc) sscanf(mcc, "\"mcc\": \"%15[^\"]\"", transaction->merchant.mcc);

        char* m_avg = strstr(ptr, "\"avg_amount\"");
        if (m_avg) sscanf(m_avg, "\"avg_amount\": %f", &transaction->merchant.avg_amount);
    }

    // Terminal
    ptr = strstr(body, "\"terminal\"");
    if (ptr) {
        char* online = strstr(ptr, "\"is_online\"");
        if (online) transaction->terminal.is_online = (strchr(online, ':') + 2)[0] == 't';

        char* card = strstr(ptr, "\"card_present\"");
        if (card) transaction->terminal.card_present = (strchr(card, ':') + 2)[0] == 't';

        char* km_home = strstr(ptr, "\"km_from_home\"");
        if (km_home) sscanf(km_home, "\"km_from_home\": %f", &transaction->terminal.km_from_home);
    }

    // Last Transaction
    ptr = strstr(body, "\"last_transaction\"");
    // Se não existir ou for null (ex: "last_transaction": null)
    if (ptr && strstr(ptr, "null") != ptr + 20) { 
        transaction->has_last_transaction = TRUE;
        
        char* ts = strstr(ptr, "\"timestamp\"");
        if (ts) sscanf(ts, "\"timestamp\": \"%31[^\"]\"", transaction->last_transaction.timestamp);
        
        char* km_curr = strstr(ptr, "\"km_from_current\"");
        if (km_curr) sscanf(km_curr, "\"km_from_current\": %f", &transaction->last_transaction.km_from_current);
    } else {
        transaction->has_last_transaction = FALSE;
    }
}

void print_transaction(TransactionRequest* req) {
    printf("=== Transaction Request ===\n");
    printf("ID: %s\n", req->id);
    
    printf("Transaction:\n");
    printf("  Amount: %.2f\n", req->transaction.amount);
    printf("  Installments: %d\n", req->transaction.installments);
    printf("  Requested At: %s\n", req->transaction.requested_at);
    
    printf("Customer:\n");
    printf("  Avg Amount: %.2f\n", req->customer.avg_amount);
    printf("  Tx Count 24h: %d\n", req->customer.tx_count_24h);
    printf("  Known Merchants Count: %d\n", req->customer.known_merchants_count);
    printf("  Known Merchants: [");
    for (int i = 0; i < req->customer.known_merchants_count; i++) {
        printf("\"%s\"%s", req->customer.known_merchants[i], 
               (i < req->customer.known_merchants_count - 1) ? ", " : "");
    }
    printf("]\n");

    printf("Merchant:\n");
    printf("  ID: %s\n", req->merchant.id);
    printf("  MCC: %s\n", req->merchant.mcc);
    printf("  Avg Amount: %.2f\n", req->merchant.avg_amount);

    printf("Terminal:\n");
    printf("  Is Online: %s\n", req->terminal.is_online ? "true" : "false");
    printf("  Card Present: %s\n", req->terminal.card_present ? "true" : "false");
    printf("  KM from Home: %.6f\n", req->terminal.km_from_home);

    printf("Has Last Transaction: %s\n", req->has_last_transaction ? "true" : "false");
    if (req->has_last_transaction) {
        printf("Last Transaction:\n");
        printf("  Timestamp: %s\n", req->last_transaction.timestamp);
        printf("  KM from Current: %.6f\n", req->last_transaction.km_from_current);
    }
    printf("===========================\n\n");
}

void on_http_request(http_s *request){


    fio_str_info_s path = fiobj_obj2cstr(request->path);

    char* path_1 = "/ready";
    char* path_2 = "/fraud-score";

    if(strcmp(path_1, path.data) == 0){
        
        printf("Requisição ready\n\n");


        request->status = 200;
        http_send_body(request, NULL, 0);
    }

    if(strcmp(path_2, path.data) == 0){
        
        printf("Requisição fraud-score\n\n");

        fio_str_info_s body = fiobj_obj2cstr(request->body);

        TransactionRequest req;

        parse_json(&req, body.data);
        
        // print_transaction(&req);

        // O cursor do ponteiro do arquivo avança até o final a cada requisição
        // Precisamos voltar ele para o início (byte 0) antes de rodar o knn de novo.
        rewind(db_file);

        int frauds = knn(K, db_file, &req);

        printf("frauds %d\n", frauds);

        float fraud_score = (float)(frauds)/K;

        char* approved = fraud_score < 0.6 ? "true" : "false";

        char response[128];

        int len = snprintf(response, sizeof(response), "{\"approved\": %s, \"fraud_score\": %.2f}", approved, fraud_score);

        request->status = 200;
        
        http_send_body(request, response, len); 
    }




}



int main(){

    db_file = fopen("resources/references.bin", "rb");
    if (!db_file) {
        printf("Não foi possível abrir resources/references.bin\n");
        return 1;
    }

    http_listen("9999", NULL, .on_request=on_http_request);
    fio_start(.threads = 1);

    if (db_file) fclose(db_file);
    return 0;

    char* merchants[] = {"MERC-008", "MERC-007", "MERC-005"};


    TransactionRequest test_req = {
        .id = "tx-3330991687",
        .transaction = {
            .amount = 9505.97f,
            .installments = 10,
            .requested_at = "2026-03-14T05:15:12Z"
        },
        .customer = {
            .avg_amount = 81.28f,
            .tx_count_24h = 20,
            .known_merchants = merchants,
            .known_merchants_count = 3
        },
        .merchant = {
            .id = "MERC-068",
            .mcc = "7802",
            .avg_amount = 54.86f
        },
        .terminal = {
            .is_online = FALSE,
            .card_present = TRUE,
            .km_from_home = 952.27f
        },
        .has_last_transaction = FALSE
    };

    FILE* file = fopen("resources/references.bin", "rb");

    printf("Fraudes: %d\n", knn(5, file, &test_req));
    

    return 0;
}