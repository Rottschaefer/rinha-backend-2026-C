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