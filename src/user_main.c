
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "c_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_common.h"
#include "lwip/tcp.h"
#include "espconn/espconn.h"
#include "espconn/espconn_tcp.h"

#include "rgb_driver.h"
#include "display_driver.h"
#include "rtc_io.h"
#include "tcp_server.h"

#include "espconn.h"

/*
DO NOT MODIFY;
NOT PART OF THE APPLICATION LOGIC; 
system function to reserve space for wifi parameters
*/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}


/*
TODO:
5. WIFI config within 5 min of boot

6. Periodic connect network and perform NTP
*/
static xQueueHandle shift_reg_quque;
static xQueueHandle rtc_output_queue;
static xQueueHandle rtc_input_queue;
static xQueueHandle rgb_input_queue;
static xQueueHandle main_clock_display_signal_queue;

enum MAIN_CLOCK_DISPALY_SIGNAL{
    MAIN_CLOCK_DISPLAY_PAUSE,
    MAIN_CLOCK_DSIPLAY_UNPAUSE
};

void main_clock_display_loop(void* arg){
    // init shift reg pin
    display_buffer_t db;
    display_driver_init(&db);
    rtc_time_t t;
    rtc_init();
    enum MAIN_CLOCK_DISPALY_SIGNAL s = MAIN_CLOCK_DSIPLAY_UNPAUSE;
    main_clock_display_signal_queue =  xQueueCreate(1, sizeof(s));
    while(1){
        if(s == MAIN_CLOCK_DSIPLAY_UNPAUSE){
            rtc_get_time(&t);
            display_driver_set(&db, 0, t.second[1]);
            display_driver_set(&db, 1, t.second[0]);
            display_driver_set(&db, 2, t.minute[1]);
            display_driver_set(&db, 3, t.minute[0]);
            display_driver_set(&db, 4, t.hour[0]);
            display_driver_set(&db, 5, t.hour[1]);
            display_driver_show(&db);
        }
        xQueueReceive(main_clock_display_signal_queue, &s, 1000/portTICK_RATE_MS);
        // display_driver_scan_loop(&db);
    }
}

// void tick_service(void* o){
//     //init i2c interface
//     rtc_output_queue = xQueueCreate(1, sizeof(rtc_time_t));
//     rtc_init();
//     struct rtc_time_t t;
//     while(1){
//         rtc_get_time(&t);
//         xQueueSend(rtc_output_queue, &t, 0);
//         printf("%d%d:", t.hour[0], t.hour[1]);
//         printf("%d%d:", t.minute[0], t.minute[1]);
//         printf("%d%d\n", t.second[0], t.second[1]);
//         vTaskDelay(1000 / portTICK_RATE_MS);
//         //get time from rtc

//     }
// }


void rgb_service(void* o){
    rgb_driver_buffer_t b;
    rgb_input_queue = xQueueCreate(1, sizeof(rgb_driver_buffer_t));
    rgb_driver_init(&b);
    while(1){
        if (pdTRUE == xQueueReceive(rgb_input_queue, &b, -1)){
            rgb_driver_show(&b);
        }
    }
}

void rgb_test(void* o){
    printf("booted\n");
    printf("cpu_freq: %d\n", system_get_cpu_freq());
    vTaskDelay(2000/portTICK_RATE_MS);
    rgb_driver_buffer_t bf;
    rgb_driver_init(&bf);
    vTaskDelay(1000/portTICK_RATE_MS);
    rgb_driver_set_all(&bf, 0, 0, 0);

    vTaskDelay(5000/portTICK_RATE_MS);
    rgb_driver_set_all(&bf, 100, 0, 0);
    vTaskDelay(1000/portTICK_RATE_MS);
    // rgb_driver_set_all(&bf, 255, 0, 0);
    // rgb_driver_set_all(&bf, 0, 0, 0);
    // uint32_t j = 0;
    // while(1){
    //     for(uint8_t i=0; i<255; i++){
    //         vTaskDelay(100/portTICK_RATE_MS);
    //         if(j == 0){
    //             rgb_driver_set_all(&bf, i, 0, 0);
    //         }else if(j == 1){
    //             rgb_driver_set_all(&bf, 0, i, 0);
    //         }else if(j == 2){
    //             rgb_driver_set_all(&bf, 0, 0, i);
    //         }
    //     }
    //     if(++j > 2) j = 0;

    // }
    while(1){}
}

void display_test(void * o){
    char i = 0;
        display_buffer_t db;
        display_driver_init(&db);


    while(1){
        i = (i==9)?0:9;
        display_driver_set(&db, 0, i);
        display_driver_show(&db);
        vTaskDelay(2000/portTICK_RATE_MS);
        // if(++i > 9) i=0;
    }
}


//TODO check input size to catch invalid cmd and avoid reading beyond (end of) buffer
void cmd_parse_loop(xQueueHandle input_queue){
    struct espconn* connection;
    struct tcp_server_line_output input;
    char* input_line;
    int scan_n;
    while(1){
        if(pdTRUE != xQueueReceive(input_queue, &input, -1)){
            continue;
        }
        input_line = input.output;
        connection = input.connection;
        // assume first byte is command
        switch(input_line[0]){
            /* TODO: implement send to rtc
            case 't':
            {
                // set time
                // time format HH:MM:SS
                int h0, h1, m0, m1, s0, s1;
                scan_n = sscanf(input_line + 1, "%1d%1d:%1d%1d:%1d%1d",
                    &h0,
                    &h1,
                    &m0,
                    &m1,
                    &s0,
                    &s1);
                if(scan_n != 6){
                    char errmsg[] = "ERROR: format error, use HH:MM:SS\n";
                    espconn_sent(connection, errmsg, sizeof(errmsg));
                }else{
                    char msg[] = "ERROR: not implemented yet\n";
                    espconn_sent(connection, msg, sizeof(msg));
                    // if(){
                        // char errmsg[] = "ERROR: rtc busy\n";
                        // espconn_sent(connection, errmsg, sizeof(errmsg));
                    // }else{
                        // TODO: send to rtc module
                        // espconn_sent(connection, TCP_SERVER_OK_RESPONSE, sizeof(TCP_SERVER_OK_RESPONSE));
                    // }
                }
                break;
            }
            */
            case 'l':
            // set rgb color
            //color format: RRGGBB (hex)
            {
                int r,g,b;
                scan_n = sscanf(input_line + 1, "%2x%2x%2x", &r, &g, &b);
                if(scan_n != 3){
                    char errmsg[] = "ERROR: format error, use RRGGBB (hex)\n";
                    espconn_sent(connection, errmsg, sizeof(errmsg));
                }else{
                    rgb_driver_buffer_t bf;
                    rgb_driver_set_all(&bf, r, g, b);
                    if(pdTRUE!=xQueueSend(rgb_input_queue, &bf, 10/portTICK_RATE_MS)){
                        char errmsg[] = "ERROR: rgb driver busy\n";
                        espconn_sent(connection, errmsg, sizeof(errmsg));
                    }else{
                        espconn_sent(connection, TCP_SERVER_OK_RESPONSE, sizeof(TCP_SERVER_OK_RESPONSE));
                    }
                }
            }
            break;
            case 'w':
            // write to rtc register
            //format: AVV (address in hex, value in hex)
            {
                int a, v;
                sscanf(input_line + 1, "%1x%2x", a, v);
                rtc_write_reg_raw((uint8_t) a, (uint8_t) v);
            }
            break;
            case 'p':
            //  pause/unpause main_clock_display
            // 0 to unpause, 1 to pause
            {
                enum MAIN_CLOCK_DISPALY_SIGNAL s = MAIN_CLOCK_DISPLAY_PAUSE;
                int ok = 1;
                if(input_line[1] == '1'){
                    s = MAIN_CLOCK_DISPLAY_PAUSE;
                }else if(input_line[1] == '0'){
                    s = MAIN_CLOCK_DSIPLAY_UNPAUSE;
                }else{
                    ok = 0;
                    char errmsg[] = "ERROR: use 0 to unpause, 1 to pause\n";
                    espconn_sent(connection, errmsg, sizeof(errmsg));
                }
                if(ok){
                    if (pdTRUE == xQueueSend(main_clock_display_signal_queue, &s, 0)){
                        espconn_sent(connection, TCP_SERVER_OK_RESPONSE, sizeof(TCP_SERVER_OK_RESPONSE));
                    }else{
                        char errmsg[] = "ERROR: main clock display loop busy\n";
                        espconn_sent(connection, errmsg, sizeof(errmsg));
                    }
                }
            }
            break;
            case 's':
            // set digit display, pause clock display first to avoid clash
            // format: VVVVVV (V: value, 0-9)
            {
                display_buffer_t db;
                display_driver_init(&db);
                int ok = 1;
                for(int i=0;i<6;i++){
                    char x = input_line[i + 1];
                    if(x == '\0'){
                        ok = 0;
                        break;
                    }else if(x == '-'){
                        display_driver_set(&db, i, -1);
                    }else if(x >= '0' && x <= '9'){
                        display_driver_set(&db, i, x - '0');
                    }else{
                        ok = 0;
                        break;
                    }
                }
                if(ok){
                    display_driver_show(&db);
                    espconn_sent(connection, TCP_SERVER_OK_RESPONSE, sizeof(TCP_SERVER_OK_RESPONSE));
                }else{
                    char errmsg[] = "ERROR: format: VVVVVV (V: value, 0-9-)\n";
                    espconn_sent(connection, errmsg, sizeof(errmsg));
                }
            }
            break;
            case 't':
            {
                rtc_time_t t;
                printf("querying rtc\n");
                rtc_get_time(&t);
                char datestr[] = "00-00-00 "; // YY-MM-DD
                char timestr[] = "00:00:00\n";
                timestr[0] = t.hour[0] + '0';
                timestr[1] = t.hour[1] + '0';
                timestr[3] = t.minute[0] + '0';
                timestr[4] = t.minute[1] + '0';
                timestr[6] = t.second[0] + '0';
                timestr[7] = t.second[1] + '0';
                espconn_sent(connection, datestr, sizeof datestr);                
                espconn_sent(connection, timestr, sizeof timestr);
            }
            break;
            default:
            {
                char errmsg[] = "ERROR: unkown command\n";
                espconn_sent(connection, errmsg, sizeof(errmsg));
            }
            
        }
        free(input_line);
    }
}

void tcp_server_test(void * a){
    xQueueHandle q = tcp_server_init();
    cmd_parse_loop(q);
    while(1){}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    espconn_init();
    system_update_cpu_freq(160);
    rgb_driver_buffer_t b;
    rgb_driver_init(&b);
    rtc_init();
    // xTaskCreate(rtc_service, "rtc service", 512, NULL, 2, NULL);

    // xTaskCreate(display_service, "display", 512, NULL, 2, NULL);
    // xTaskCreate(display_test, "dts", 512, NULL, 2, NULL);
    // xTaskCreate(rgb_service, "rgb", 1024, NULL, 2, NULL);
    xTaskCreate(tcp_server_test, "tcpst", 1024, NULL, 2, NULL);
}

