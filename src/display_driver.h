#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

/*
set BOARD_TYPE to 0 for nx_clk board
set BOARD_TYPE 1 to for amberGlow
nx_clk sends all data out to shift register at once when display_driver_show is called,
display_driver_show does not need to be called between dispaly updates

nixie tube on amberGlow are multiplexed, dispaly_driver_show should be call continuously in succession to maintain 
digit display
*/

#define BOARD_TYPE 0
/*
amberGlow shift register bit structure
cathod(segment) driver: 
    there are 2 rows and 5 column, row 2 has 1 extra column (column 6) to control the decimal point
    rows are active-high, and comumn are active-low. (i.e to writing high to a row and low to a column activate the corresponding segment)
    bit 0 and bit 1 of the shift register are connected to row1 and row 2 respectively
    bit [3:7] are connected to row 1 to 6 respectively

anode(tube) driver:
    bit [0:5] are connected to tube 1-6 respectively
    tubes are active-high (i.e writing 1 to the corresponding bit signals 1)
*/
#define DISPLAY_ALL_CATHODE_OFF 0x1F
#define DISPLAY_ALL_ANODE_OFF 0x00

/*
wiring:
(d1 mini pin)|(ESP8266 PIN)|(pcb net name)|(description)
    D0       |     GPIO16  |     SOE      |  SOE is connected to the enable pin of all the shift regs
                                          | SOE line is active-low and has pull up resistor on pcb,
    D5       |     GPIO14  |     SECLK    | SECLK is the clock for data input
    D7       |     GPIO13  |     SER      | SER is the data input to the first shift reg in the chain
    D8       |     GPIO15  |     RCLK     | RCLK is the line for latching output
*/

#define DISPLAY_SECLK_GPIO 14
#define DISPLAY_SER_GPIO 13
#define DISPLAY_RCLK_GPIO 15

#define DISPLAY_SECLK_FUNC_SEL FUNC_GPIO14
#define DISPLAY_SER_FUNC_SEL FUNC_GPIO13
#define DISPLAY_RCLK_FUNC_SEL FUNC_GPIO15


/*
row are connected to q1, q2  of shift register,(note that q0 also exists)
high level enables the row
hince 1 << 1 for 1st row, 1 << 2 for 2nd row
*/
#define LED_ROW(r) ((1 << (r)) & 0x06)
/*
low level enables the column,
column 1 - 5 are connected to q3 - q7
hince
column 1: (~(1 << 3)) & 0xf8
column 2: (~(1 << 4)) & 0xf8

ref:
byte map (X: don't care, R: row, C: Column, H: active high, L: active low)
7 6 5 4 3 2 1 0
C C C C C R R X
L L L L L H H X
Note that bit should be sent in reverse order
*/

#define LED_COL(c) ((~(1 << ((c) + 2))) & 0xF8)




// for nx_clk (in-14)
#if BOARD_TYPE == 0
static uint8_t number_code_table[10] = {
    LED_ROW(2) | LED_COL(1), //0  
    LED_ROW(1) | LED_COL(1), //1
    LED_ROW(1) | LED_COL(2), //2
    LED_ROW(1) | LED_COL(3), //3
    LED_ROW(1) | LED_COL(4), //4
    LED_ROW(1) | LED_COL(5), //5
    LED_ROW(2) | LED_COL(5), //6  
    LED_ROW(2) | LED_COL(4), //7
    LED_ROW(2) | LED_COL(3), //8
    LED_ROW(2) | LED_COL(2)  //9  
};

struct display_buffer_t
{
    uint8_t cathod_shift_reg_buffer[6];
};

#else

struct display_buffer_t
{
    uint8_t cathod_shift_reg_buffer[6];
    uint8_t anode_shift_reg_buffer[6];
    uint8_t scan_pointer;
    char latch_ready;
    char is_paused;
    uint32_t hold_time; 
    uint32_t last_hold_start;
};
#endif


typedef struct display_buffer_t display_buffer_t;

void display_driver_init(display_buffer_t* db);
void display_driver_show(display_buffer_t * db);
void display_driver_set(display_buffer_t * db, uint8_t index, uint8_t number);
#endif
