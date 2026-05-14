// #include <http.h>
#include <stdio.h>
#include <string.h>
#include "vec_search/vec_search.h"
#include <time.h>

#define TRUE 1
#define FALSE 0

typedef struct __attribute__((packed)) {
float coords[14];
unsigned char label;
unsigned char padding[7];
} Record;

// void on_http_request(http_s *request){

//     fio_str_info_s path = fiobj_obj2cstr(request->path);

//     char* path_1 = "/ready";
//     char* path_2 = "/fraud-score";

//     if(strcmp(path_1, path.data) == 0){
        
//         printf("Requisição ready\n\n");


//         request->status = 200;
//         http_send_body(request, NULL, 0);
//     }

//     if(strcmp(path_2, path.data) == 0){
        
//         printf("Requisição fraud-score\n\n");

//         request->status = 200;

//         char* response = "{\"approved\": false, \"fraud_score\": 0.8}";
        
//         http_send_body(request, response, 39);
//     }




// }



int main(){

    // http_listen("9999", NULL, .on_request=on_http_request);
    // fio_start(.threads = 1);

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