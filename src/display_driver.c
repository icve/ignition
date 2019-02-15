#include "c_types.h"
#include <gpio.h>
#include "esp_misc.h"
#include <display_driver.h>
#include "esp8266/pin_mux_register.h"


/* TODO: go fast!!!
    precompute bit mask and write directly to register
*/

/*
send n number of bytes to shift reg, without latching
note that both byte order and bit order are reversed
*/

void display_driver_send_to_shift_reg(uint8* buf, int n){
    for(int i=n-1;i >= 0;i--){
        for(int j=7; j >= 0 ;j--){
            uint8 b = (buf[i]>>j) & 1;
            GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_SER_GPIO), b);
            GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_SECLK_GPIO), 1);
            os_delay_us(1);
            GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_SECLK_GPIO), 0);
            os_delay_us(1);
        }
    }

}
static inline void display_driver_latch(){
    GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_RCLK_GPIO), 1);
    os_delay_us(1);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_RCLK_GPIO), 0);
    os_delay_us(1);
}

static inline void display_driver_signal_line_init(){
    PIN_FUNC_SELECT(GPIO_PIN_REG(DISPLAY_SECLK_GPIO), DISPLAY_SECLK_FUNC_SEL);
    PIN_FUNC_SELECT(GPIO_PIN_REG(DISPLAY_SER_GPIO), DISPLAY_SER_FUNC_SEL);
    PIN_FUNC_SELECT(GPIO_PIN_REG(DISPLAY_RCLK_GPIO), DISPLAY_RCLK_FUNC_SEL);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_SECLK_GPIO), 0);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_SER_GPIO), 0);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(DISPLAY_RCLK_GPIO), 0);
}

static inline void dispaly_driver_enable(){
    gpio16_output_conf();
    gpio16_output_set(0);
}

// start of nx_clk specfic driver
#if BOARD_TYPE == 0
    void display_driver_init(display_buffer_t* db){

        for(int i=0;i<6;i++){
            db->cathod_shift_reg_buffer[i] = DISPLAY_ALL_CATHODE_OFF;
            // db->anode_shift_reg_buffer[i] = 0x80 >> i; 
        }
        // db->latch_ready = 0;
        // db->scan_pointer = 0;
        // db->is_paused = 0;
        // db->hold_time = 1000;
        // init shift_reg_io_lines
        display_driver_signal_line_init();

        //send init data to shift reg
        // display_driver_send_to_shift_reg(db->anode_shift_reg_buffer, 1);
        display_driver_send_to_shift_reg(db->cathod_shift_reg_buffer, 6);
        display_driver_latch();
        dispaly_driver_enable();

    }
    void display_driver_set(display_buffer_t * db, uint8_t index, uint8_t number){
        printf("n: %d, code: %x\n", number, number_code_table[number]);
        db->cathod_shift_reg_buffer[index] = number_code_table[number];
    }
    void display_driver_show(display_buffer_t * db){
        display_driver_send_to_shift_reg(db->cathod_shift_reg_buffer, 6);
        display_driver_latch();
    }
#else // amberGlow driver

    void display_driver_init(display_buffer_t* db){

        for(int i=0;i<6;i++){
            db->cathod_shift_reg_buffer[i] = DISPLAY_ALL_CATHODE_OFF;
            db->anode_shift_reg_buffer[i] = 0x80 >> i; 
        }
        db->latch_ready = 0;
        db->scan_pointer = 0;
        db->is_paused = 0;
        db->hold_time = 1000;
        // init shift_reg_io_lines
        display_driver_signal_line_init();

        // send init data to shift reg
        display_driver_send_to_shift_reg(db->anode_shift_reg_buffer, 1);
        display_driver_send_to_shift_reg(db->cathod_shift_reg_buffer, 1);
        display_driver_latch();
        dispaly_driver_enable();

    }

    static inline void display_driver_pause(display_buffer_t* db){
        db->is_paused = 1;
    }

    static void inline display_dirver_unpause(display_buffer_t* db){
        db->is_paused = 0;
    }

    void display_driver_scan_loop(display_buffer_t* db){
        if(db->is_paused) return;
        if(db->latch_ready){
            uint32 t = system_get_time();
            if(t - db->last_hold_start >= db->hold_time){
                display_driver_latch();
                db->latch_ready = 0;
            }
        }else{
            display_driver_send_to_shift_reg(&db->anode_shift_reg_buffer[db->scan_pointer], 1);
            display_driver_send_to_shift_reg(&db->cathod_shift_reg_buffer[db->scan_pointer], 1);
            if(db->scan_pointer < 5){
                db->scan_pointer++;
            }else{
                db->scan_pointer = 0;
            }
        }
    }

#endif //end of board select
