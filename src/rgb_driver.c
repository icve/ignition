
#include <esp8266/pin_mux_register.h>
#include <rgb_driver.h>
#include <c_types.h>
#include <gpio.h>
#include "freertos/FreeRTOS.h"

#include "freertos/task.h"
#include "esp8266/ets_sys.h"
#include "esp8266/eagle_soc.h"

void rgb_driver_init(rgb_driver_buffer_t* bf){
    PIN_FUNC_SELECT(GPIO_PIN_REG(RGB_SIGNAL_GPIO), RGB_SIGNAL_FUNC_SEL);
    RGB_GPIO_WRITE(0);
    // rgb_driver_set_all(bf, 0, 0, 0);
    vTaskDelay(10/portTICK_RATE_MS);
}

static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}

/*
potential issues
// 1.os tick not stopped
// 2.counts starts after write
3.large bit shift
// 4.high low time
5. cache function
6. gpio set function
*/
void IRAM_ATTR rgb_driver_show(rgb_driver_buffer_t* bf){
    //DEGBUGING: write only 24 bit
        //DEBUGGING only send 0 int
        // printf("pxval: %x\n", pixel_val);
        // printf("bits:");
        // gpio_output_conf(0, 1, 1, 0);
        uint32_t t0h, t1h, thl, pixel_val, high_time, t, mask, pin_mask, chian_len;
        t0h = RGB_T0H_CC;
        t1h = RGB_T1H_CC;
        thl = RGB_HL_CC;
        high_time = t1h;
        mask = 1 << 23;
        pin_mask = 1 << RGB_SIGNAL_GPIO;
        chian_len = RGB_CHAIN_LENGTH;

        uint32_t i, j;
        i=0;
        j=0;
        taskENTER_CRITICAL();
        ETS_INTR_LOCK();
        t = _getCycleCount();
        uint8_t first_run = 1;
    for(; i< chian_len; i++){
        // printf("led%d\n", i);
        pixel_val = bf->buf[i];
        // t = _getCycleCount();
        // for(int i=0;i<6;i++){
        for(j=0; j<24;){
            // if((pixel_val & mask)){
            //     // 1
            //     // printf("b:1, ");
            //     high_time = RGB_T1H_CC;
            // }else{
            //     //0
            //     // printf("b:0, ");
            //     high_time = RGB_T0H_CC;

            // }
            // x = _getCycleCount();
            high_time = (pixel_val & mask) ? t1h: t0h;
            pixel_val <<= (!first_run) ? 1:0;
            // printf("ck%d\n", _getCycleCount() - x);
            
            // printf("h:%d, l:%d\n", high_time, RGB_HL_CC - high_time);
            // printf("extra clock 0: %d\n", _getCycleCount() - c);

            while((_getCycleCount() - t) < RGB_HL_CC);
            // printf("ck%d\n", _getCycleCount() - t);
            t = _getCycleCount();
            // x = _getCycleCount();
            // RGB_GPIO_WRITE(1);
            if(!first_run)
            GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pin_mask);       // Set high
            // gpio_output_conf(1, 0, 0, 0);
            // printf("ht: %d\n", _getCycleCount() - x);
            while((_getCycleCount() - t) < high_time);
            // x = _getCycleCount();
            // RGB_GPIO_WRITE(0);
            if(!first_run)
            GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pin_mask);       // Set low
            // gpio_output_conf(0, 1, 0, 0);
            // printf("lt: %d\n", _getCycleCount() - x);

            // printf("extra clock 1: %d\n", c - high_time);
            // c = _getCycleCount();
            // while(((c = _getCycleCount()) - t) < (low_time-7)){
            //     //busy wait
            // }
            j += (!first_run) ? 1 : 0;
            first_run = 0;
        }
    }
        // }
        // printf("\n");
    // }
    ETS_INTR_UNLOCK();
    taskEXIT_CRITICAL();
    // RGB_GPIO_WRITE(1);
    // RGB_GPIO_WRITE(0);
}

void rgb_driver_set_all(rgb_driver_buffer_t* bf, uint8_t r, uint8_t g, uint8_t b){
    for(int i=0 ;i<RGB_CHAIN_LENGTH; i++){
        uint32_t val = r << (8 * RGB_RED_INDEX);
        val |= g << (8 * RGB_GREEN_INDEX);
        val |= b << (8 * RGB_BLUE_INDEX);
        bf->buf[i] = val;
        printf("%d, %x\n", i, val);
    }
}