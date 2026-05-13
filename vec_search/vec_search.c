#include "vec_search.h"
#include <stdio.h>
#include <limits.h>


typedef struct __attribute__((packed)) {
float coords[14];
unsigned char label;
unsigned char padding[7];
} Record;

typedef struct vizinho{
    unsigned char label;
    float dist;
}Vizinho;



float calc_dist(float* a, float* b){
    //Aqui podemos otimizar, mudando o método de cálculo, por exemplo 

    float total = 0;
    for (int i = 0; i < 14; i++)
    {
        total += ((a[i] - b[i])*(a[i] - b[i]));
    }

    return total;
    
}

int knn(int k, FILE* file, float* vetor){
    //Passa por cada registro do arquivo calculando a distancia e guardando os k mais pertos. Podemos otimizar essa leitura 

    Vizinho top_k[k];

    for (int i = 0; i < k; i++)
    {   
        top_k[i].label = 0;
        top_k[i].dist = __FLT_MAX__;
    }

    Record r;
    float dist;

    while(fread(&r, sizeof(Record), 1, file) == 1){
        dist = calc_dist(vetor, r.coords);

        if(dist < top_k[k-1].dist){

            int pos = k-1;
            
            while (pos > 0 && dist < top_k[pos-1].dist)
            {   

                top_k[pos] = top_k[pos-1];
                pos --;
            }

            top_k[pos].dist = dist;
            top_k[pos].label = r.label;
        }
        
    }

    //frauds é label 0
    int frauds = 0;

    for (int i = 0; i < k; i++)
    {
        frauds += !(top_k[i].label);
    }
    
    return frauds;


}

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


void vectorize_request(TransactionRequest* req, Normalization* norm, float* out) {}