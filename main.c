#include <http.h>
#include <stdio.h>
#include <string.h>

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

        request->status = 200;

        char* response = "{\"approved\": false, \"fraud_score\": 0.8}";
        
        http_send_body(request, response, 39);
    }




}

int main(){

    http_listen("9999", NULL, .on_request=on_http_request);
    fio_start(.threads = 1);
    printf("Hello");
    return 0;
}