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

    char* merchants[] = {"MERC-009", "MERC-001", "MERC-001"};


    TransactionRequest test_req = {
        .id = "tx-3576980410",
        .transaction = {
            .amount = 384.88f,
            .installments = 3,
            .requested_at = "2026-03-3T20:23:35Z"
        },
        .customer = {
            .avg_amount = 769.76f,
            .tx_count_24h = 3,
            .known_merchants = merchants,
            .known_merchants_count = 3
        },
        .merchant = {
            .id = "MERC-001",
            .mcc = "5912",
            .avg_amount = 298.95f
        },
        .terminal = {
            .is_online = FALSE,
            .card_present = TRUE,
            .km_from_home = 13.7090520965f
        },
        .has_last_transaction = TRUE,
        .last_transaction = {
            .timestamp = "2026-03-11T14:58:35Z",
            .km_from_current = 18.8626479774f
        }
    };

    float vec[14];

    vectorize_request(&test_req, vec);

    for (int i = 0; i < 14; i++)
    {
        printf("%f - ", vec[i]);
    }

    printf("\n");
    

    return 0;
}