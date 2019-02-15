
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
// 1. digit decoder byte to byte state

2. spi bit bang (spi data queue) (potential timing concern)

3. I2C communication with RTC module 

4. pwm led

5. WIFI config within 5 min of boot

6. Periodic connect network and perform NTP
*/
#define PIN_SOE 0
#define PIN_SECLK_SCK 5
#define PIN_SER_MOSI 7
#define PIN_RCLK_SS 8
#define PIN_RCLK_SS_2 3

//RTC i2c pin def
#define PIN_SCL 1
#define PIN_SDA 2

#define PIN_ILED 4

#define RTC_I2C_PORT I2C_NUM_0

static xQueueHandle shift_reg_quque;
static xQueueHandle rtc_output_queue;
static xQueueHandle rtc_input_queue;
static xQueueHandle rgb_input_queue;


void display_service(void* arg){
    // init shift reg pin
    display_buffer_t db;
    display_driver_init(&db);
    rtc_time_t t;
    while(1){
        if(xQueueReceive(rtc_output_queue, &t, 0) == pdTRUE){
            display_driver_set(&db, 5, t.second[1]);
            display_driver_set(&db, 4, t.second[0]);
            display_driver_set(&db, 3, t.minute[1]);
            display_driver_set(&db, 2, t.minute[0]);
            display_driver_set(&db, 1, t.hour[0]);
            display_driver_set(&db, 0, t.hour[1]);
        }
        display_driver_show(rtc_output_queue);
        vTaskDelay(500/portTICK_RATE_MS);
        // display_driver_scan_loop(&db);
    }
}

void rtc_service(void* o){
    //init i2c interface
    rtc_output_queue = xQueueCreate(1, sizeof(rtc_time_t));
    rtc_output_queue = xQueueCreate(1, sizeof(rtc_time_t));
    rtc_init_i2c();
    struct rtc_time_t t;
    while(1){
        rtc_get_time(&t);
        xQueueSend(rtc_output_queue, &t, 0);
        printf("%d%d:", t.hour[0], t.hour[1]);
        printf("%d%d:", t.minute[0], t.minute[1]);
        printf("%d%d\n", t.second[0], t.second[1]);
        vTaskDelay(1000 / portTICK_RATE_MS);
        //get time from rtc

    }
}


void rgb_service(void* o){
    rgb_driver_buffer_t b;
    rgb_input_queue = xQueueCreate(1, sizeof(rgb_driver_buffer_t));
    while(1){
        if (pdTRUE == xQueueReceive(rgb_input_queue, &b, -1)){
            display_driver_show(&b);
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

void tcp_server_test(){
    tcp_server_init();
    while(1){
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void cmp_parse_service(){
    //TODO get input from server
    struct espconn* connection;
    struct tcp_server_line_output input;
    char* input_line;
    int scan_n;
    while(1){
        if(pdTRUE != xQueueReceive(TCP_SERVER_PROCESSING_QUEUE, &input, -1)){
            continue;
        }
        input_line = input.output;
        // assume first byte is command
        switch(input_line[0]){
            case 't':
            {
                // set time
                // time format HH:MM:SS
                rtc_time_t time;
                scan_n = sscanf(input_line + 1, "%1d%1d:%1d%1d:%1d%1d",
                    time.hour[0],
                    time.hour[1],
                    time.minute[0],
                    time.minute[1],
                    time.hour[0],
                    time.hour[1]);
                if(scan_n != 6){
                    char errmsg[] = "ERROR: format error, use HH:MM:SS\n";
                    espconn_sent(connection, errmsg, sizeof(errmsg));
                }else{
                    if(pdTRUE != xQueueSend(rtc_input_queue, &time, 0)){
                        char errmsg[] = "ERROR: rtc busy\n";
                        espconn_sent(connection, errmsg, sizeof(errmsg));
                    }else{
                        espconn_sent(connection, TCP_SERVER_OK_RESPONSE, sizeof(TCP_SERVER_OK_RESPONSE));
                    }
                }
                break;
            }
            case 'l':
            {
                // set rgb color
                //color format: RRGGBB (hex)
                int r,g,b;
                scan_n = sscanf(input_line + 1, "%2x%2x%2x", r, g, b);
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
                break;
            }
            default:
            {
                char errmsg[] = "ERROR: unkown command\n";
                espconn_sent(connection, errmsg, sizeof(errmsg));
            }
            
        }
        free(input_line);
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    system_update_cpu_freq(160);
    // xTaskCreate(rtc_service, "rtc service", 512, NULL, 2, NULL);
    // xTaskCreate(display_service, "display", 512, NULL, 2, NULL);
    // xTaskCreate(display_test, "dts", 512, NULL, 2, NULL);
    // xTaskCreate(rgb_test, "rgb", 1024, NULL, 2, NULL);
    xTaskCreate(tcp_server_test, "tcpst", 512, NULL, 2, NULL);
}

