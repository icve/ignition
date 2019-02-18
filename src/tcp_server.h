#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#define TCP_SERVER_LISTEN_PORT 6666

#define TCP_SERVER_LINE_BUFFER_SIZE 100


struct tcp_server_line_buffer{
    int current_size;
    char ongoing_connection;
    char line[TCP_SERVER_LINE_BUFFER_SIZE];
    char ip_addr[4];
    int port;
};

struct tcp_server_line_output{
    struct espconn* connection;
    char* output;
};


static char TCP_SERVER_OK_RESPONSE[] = "OK\n";


static struct tcp_server_line_buffer* TCP_SERVER_LINE_BUFFER = NULL;
static xQueueHandle TCP_SERVER_PROCESSING_QUEUE = NULL;
static xSemaphoreHandle TCP_SERVER_LINE_BUFFER_SEMAPHORE = NULL;

xQueueHandle tcp_server_init();


#endif
