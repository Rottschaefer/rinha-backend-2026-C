#include <http.h>
#include <stdio.h>
#include <string.h>
#include "req_2_vec/req_2_vec.h"
#include <time.h>
#include "usearch/usearch.h"
#include <assert.h> 

#define TRUE 1
#define FALSE 0
#define K 9

typedef struct __attribute__((packed)) {
float coords[14];
unsigned char label;
unsigned char padding[7];
} Record;


#define VECTORS_COUNT 3000000

usearch_index_t global_index = NULL;

unsigned char global_labels[VECTORS_COUNT]; 

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

        usearch_error_t error = NULL;

        fio_str_info_s body = fiobj_obj2cstr(request->body);

        TransactionRequest req;


        parse_json(&req, body.data);

        float vector[14];


        vectorize_request(&req, vector);
        
        usearch_key_t found_keys[K];
        usearch_distance_t found_distances[K];
        size_t found_count = usearch_search(
            global_index, &vector[0], usearch_scalar_f32_k, K,
            &found_keys[0], &found_distances[0], &error);
        
        int frauds = 0;

        for (int i = 0; i < K; i++)
        {
            frauds += global_labels[found_keys[i]];
        }
        

        printf("frauds %d\n", frauds);

        float fraud_score = (float)(frauds)/K;

        char* approved = fraud_score < 0.35 ? "true" : "false";

        char response[128];

        int len = snprintf(response, sizeof(response), "{\"approved\": %s, \"fraud_score\": %.2f}", approved, fraud_score);

        request->status = 200;
        
        http_send_body(request, response, len); 
    }




}



int main(int argc, char* argv[]){
    size_t dimensions = 14;

    float vector[dimensions];

    usearch_error_t error = NULL;

    //Inicializa as metadados do usearch index

    usearch_init_options_t opts = {
        .metric_kind = usearch_metric_cos_k,
        .quantization = usearch_scalar_f32_k, // or f32_k, bf16_k, e5m2_k, e4m3_k, e3m2_k, e2m3_k, i8_k, u8_k
        .dimensions = dimensions,
        .expansion_add = 0, // for defaults
        .expansion_search = 64 // for defaults
    };
    global_index = usearch_init(&opts, &error);


    //Carrega o index em disco para memória

    usearch_load(global_index, "index.usearch", &error);
    if (error) goto cleanup;



    //Preenche o global_labels com os dados do arquivo labels
    FILE* labels = fopen("resources/labels", "rb");
    if (!labels) {
        printf("Não foi possível abrir o arquivo com as labels (resources/references.bin)\n");
        goto cleanup;
    }

    printf("Carregando labels no vetor...\n");

    unsigned char label;
    int i =0;
    while( fread(&label, sizeof(char), 1, labels) == 1){
        global_labels[i] = label;
        i++;
    }

    fclose(labels);



    printf("API rodando na porta %s\n", argv[1]);


    http_listen(argv[1], NULL, .on_request=on_http_request);
    fio_start(.threads = 1);

    return 0;


    cleanup:
        if (error) fprintf(stderr, "Error: %s\n", error);
        if (global_index) usearch_free(global_index, &error);
        return error ? 1 : 0;
}