
#include <esp8266/pin_mux_register.h>
#include <c_types.h>
#define RGB_SIGNAL_GPIO 0
#define RGB_SIGNAL_FUNC_SEL FUNC_GPIO0

#define RGB_GPIO_WRITE(b) GPIO_OUTPUT_SET(GPIO_ID_PIN(RGB_SIGNAL_GPIO), b);

/*
timing definition
to signaling 0 or 1 to rgb led
requires making the singal line high , then low

to signal 0, keep singal line High for RGB_T0H nano second then keep low for RGB_T0L nano second
to reset keep low for RGB_RESET MICRO SECOND

*/
// clock counts, assume 80 Mhz clock
// #define RGB_T0H_CC (uint32_t)(400.0 / 12.5) 
// #define RGB_T1H_CC (uint32_t)(850.0 / 12.5)
// #define RGB_HL_CC (uint32_t)(1250.0 / 12.5)
// 160 Mhz
#define RGB_T0H_CC (uint32_t)(400.0 / 6.25) 
#define RGB_T1H_CC (uint32_t)(850.0 / 6.25)
#define RGB_HL_CC (uint32_t)(1250.0 / 6.25)


#define RGB_RESET 200

#define RGB_CHAIN_LENGTH 6
#define RGB_RED_INDEX 2
#define RGB_GREEN_INDEX 1
#define RGB_BLUE_INDEX 0

// set 1 to wait for reset before returing
#define RGB_WAIT_FOR_RESET 0

// each pixel(RGB LED) a uint32_t value
struct rgb_driver_buffer_t{
    uint32_t buf[RGB_CHAIN_LENGTH];
};

typedef struct rgb_driver_buffer_t rgb_driver_buffer_t;
void rgb_driver_init(rgb_driver_buffer_t* bf);
void rgb_driver_set_all(rgb_driver_buffer_t* bf, uint8_t r, uint8_t g, uint8_t b);
void rgb_dirver_show(rgb_driver_buffer_t* bf);