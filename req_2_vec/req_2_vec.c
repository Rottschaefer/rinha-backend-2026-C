#include "req_2_vec.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


//Usar __attribute__((packed)) faz diferença?
typedef struct {
float coords[14];
unsigned char label;
unsigned char padding[7];
} Record;

typedef struct vizinho{
    unsigned char label;
    float dist;
}Vizinho;

typedef struct {

    struct tm calendar;
    time_t timestamp;

} ParsedDate;



Normalization get_normalization_data(void) {
    Normalization norm = {
        .max_amount = 10000.0f,
        .max_installments = 12,
        .amount_vs_avg_ratio = 10.0f,
        .max_minutes = 1440,
        .max_km = 1000.0f,
        .max_tx_count_24h = 20,
        .max_merchant_avg_amount = 10000.0f
    };
    return norm;
}

float get_mcc_risk(const char* mcc) {
   
    int mcc_val = atoi(mcc);
    
    switch(mcc_val) {
        case 5411: return 0.15f;
        case 5812: return 0.30f;
        case 5912: return 0.20f;
        case 5944: return 0.45f;
        case 7801: return 0.80f;
        case 7802: return 0.75f;
        case 7995: return 0.85f;
        case 4511: return 0.35f;
        case 5311: return 0.25f;
        case 5999: return 0.50f;
        default:   return 0.50f; // Risco padrão/médio caso venha um MCC desconhecido
    }
}

// Função para manter o valor normalizado entre 0.0 e 1.0 (Clamp)
float clamp(float val) {
    if (val < 0.0f) return 0.0f;
    if (val > 1.0f) return 1.0f;
    return val;
}

int merchant_is_known(char* id, CustomerInfo* info){
    for (int i = 0; i < info->known_merchants_count; i++)
    {
        if(strcmp(id,info->known_merchants[i]) == 0) return 0;
    }

    return 1;

    //1 se merchant.id não estiver em customer.known_merchants, senão 0 (invertido: 1 = desconhecido)
    
}

ParsedDate parse_date(const char *datetime) {

    int year, month, day;
    int hour, min, sec;

    sscanf(datetime,
           "%d-%d-%dT%d:%d:%dZ",
           &year, &month, &day,
           &hour, &min, &sec);

    struct tm t = {0};

    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;

    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;

    time_t ts = timegm(&t);

    ParsedDate output = {.calendar = t, .timestamp = ts};

    return output;
}


void vectorize_request(TransactionRequest* req, float* out) {

    Normalization norm = get_normalization_data();

    out[0] = clamp(req->transaction.amount/norm.max_amount);

    out[1] = clamp((float)(req->transaction.installments)/norm.max_installments);

    out[2] = clamp((req->transaction.amount/req->customer.avg_amount)/norm.amount_vs_avg_ratio);


    ParsedDate transaction_date = parse_date(req->transaction.requested_at);

    out[3] = ((float)transaction_date.calendar.tm_hour)/23;


    out[4] = (float)(((transaction_date.calendar.tm_wday + 6)%7)/6.0); //A lib time tem domingo como 0, mas a rinha pede segunda como 0

    if(req->has_last_transaction == 0){
        out[5] = -1;
        out[6] = -1;
    }
    else{
        ParsedDate last_transaction_date = parse_date(req->last_transaction.timestamp);

        out[5] = clamp((float)(difftime(transaction_date.timestamp, last_transaction_date.timestamp)/60.0)/norm.max_minutes);
        out[6] = clamp(req->last_transaction.km_from_current/norm.max_km);
    }

    out[7] = clamp(req->terminal.km_from_home/norm.max_km);

    out[8] = clamp((float)(req->customer.tx_count_24h)/norm.max_tx_count_24h);

    out[9] = (req->terminal.is_online);

    out[10] = (req->terminal.card_present);

    
    out[11] = (merchant_is_known(req->merchant.id, &(req->customer)));

    out[12] = get_mcc_risk(req->merchant.mcc);

    out[13] = clamp(req->merchant.avg_amount/norm.max_merchant_avg_amount);
}