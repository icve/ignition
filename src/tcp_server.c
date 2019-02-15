#include "stdlib.h"
#include "lwip/tcp.h"
#include "espconn/espconn.h"
#include "espconn/espconn_tcp.h"
#include "tcp_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*
only one concurrent connection is supported, if more than one device
connects, both drops
*/
/*
TODO: race condition warning: if two machine connect simultaneously
both will accept and currupt the line buffer
solution: mutex lock & unlock before accessing line buf
*/

void data_handler(void* arg, char* pdata, unsigned short len){
    struct espconn *connection = arg;
    struct tcp_server_line_buffer* lb = TCP_SERVER_LINE_BUFFER;

    // check if current connection is the connection init the line buffer
    // char is_same_ip = memcmp(lb->ip_addr, connection->proto.tcp->remote_ip, 4) == 0;
    // char is_same_port = lb->port == connection->proto.tcp->remote_port;
    // if(is_same_ip && is_same_port){

    // }
    if(pdTRUE != xSemaphoreTake(TCP_SERVER_LINE_BUFFER_SEMAPHORE, 100/portTICK_RATE_MS)){
        printf("line buffer busy, data discarded: %.*s\n", len, pdata);
        return;
    }
    for(unsigned int i=0; i < len; i++){
        if(pdata[i] == '\n'){
            // reject empty line
            if(lb->current_size > 0){
                // reserve 1 char for line end
                char* l = malloc(lb->current_size + 1);
                memcpy(l, TCP_SERVER_LINE_BUFFER->line, lb->current_size);
                l[lb->current_size] = '\0';
                struct tcp_server_line_output lo = {
                    .connection = connection,
                    .output = l
                };
                if (pdTRUE != xQueueSend(TCP_SERVER_PROCESSING_QUEUE, &lo, 50 / portTICK_RATE_MS)){
                    printf("parser failed to catch up!\n");
                    printf("line: %s", l);
                    free(l);
                }
            }
            lb->current_size = 0;
        }else if(TCP_SERVER_LINE_BUFFER_SIZE > lb->current_size){
            lb->line[lb->current_size++] = pdata[i];
        }else{
            char errmsg[] = "ERROR: line buffer full, truncating";
            espconn_sent(connection, errmsg, sizeof(errmsg));
            lb->current_size = 0;
        }
    }
    xSemaphoreGive(TCP_SERVER_LINE_BUFFER);
}

void disconnect_handler(void *arg){
    // struct espconn *connection = arg;
    struct tcp_server_line_buffer* lb = TCP_SERVER_LINE_BUFFER;
    lb->ongoing_connection = 0;
    printf("client disconnected!\n");

}

void brocken_connection_handler(void * arg){
    // struct espconn *connection = arg;
    struct tcp_server_line_buffer* lb = TCP_SERVER_LINE_BUFFER;
    lb->ongoing_connection = 0;
    printf("connection brocken!\n");

}

void connect_handler(void *arg)
{
    struct espconn *connection = arg;
    struct tcp_server_line_buffer* lb = TCP_SERVER_LINE_BUFFER;
    if (pdTRUE != xSemaphoreTake(TCP_SERVER_LINE_BUFFER_SEMAPHORE, 0)){
        printf("concurrent connection!\n");
        return;
    }
    // only alow one concurrent connection
    if(lb->ongoing_connection){
        char errmsg[] = "ERROR: concurrent conntion not allowed!";
        espconn_sent(connection, errmsg, sizeof(errmsg));
    }else{
        // new client connets, setup line bufer
        lb->ongoing_connection = 1;
        lb->current_size = 0;
        memcpy(lb->ip_addr, connection->proto.tcp->remote_ip, 4);
        lb->port = connection->proto.tcp->remote_port;

        espconn_regist_recvcb(connection, data_handler);
        // espconn_regist_reconcb(pesp_conn, webserver_recon);
        espconn_regist_disconcb(connection, disconnect_handler);
    }
    xSemaphoreGive(TCP_SERVER_LINE_BUFFER_SEMAPHORE);
}

void tcp_server_init(){
    struct espconn *c = calloc(1, sizeof *c);
    esp_tcp *esptcp = calloc(1, sizeof *esptcp);
    c->type = ESPCONN_TCP;
    c->state = ESPCONN_NONE;
    c->proto.tcp = esptcp;
    c->proto.tcp->local_port = TCP_SERVER_LISTEN_PORT;
    TCP_SERVER_LINE_BUFFER = calloc(1, sizeof *TCP_SERVER_LINE_BUFFER);
    vSemaphoreCreateBinary(TCP_SERVER_LINE_BUFFER_SEMAPHORE);
    espconn_regist_connectcb(c, connect_handler);
    espconn_accept(c);
    printf("accept does not block!\n");
}