#include "c_types.h"
#include "i2c_master.h"
#include "rtc_io.h"

#include "stdio.h"
#define RTC_DEVICE_ADDRESS 0x68
/**
 * @brief i2c master initialization
 */
int rtc_init_i2c(void)
{
    printf("init\n");
    i2c_master_gpio_init();
    return 0;
}

/**
 * @brief test code to write
 *
 * 1. send data
 * ___________________________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | write data_len byte + ack  | stop |
 * --------|---------------------------|-------------------------|----------------------------|------|
 *
 * @param i2c_num I2C port number
 * @param reg_address slave reg address
 * @param data data to send
 * @param data_len data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */

// static int rtc_write(uint8_t dev_address, uint8_t reg_address, uint8_t *data, size_t data_len)
// {
//     i2c_master_start();
//     i2c_master_sendByte(dev_address << 1 | 0);
//     if( ! i2c_master_checkAck())
//     {
//         i2c_master_stop();
//         return 1;
//     }
//     for(int i=0; i < data_len; i++)
//     {
//         i2c_master_writeByte(data[i]);
//         if( ! i2c_master_checkAck())
//         {
//             i2c_master_stop();
//             return 1;
//         }
     
//     }
//        i2c_master_stop();
//         return 0;

// }

/**
 * @brief test code to read mpu6050
 *
 * 1. send reg address
 * ______________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | stop |
 * --------|---------------------------|-------------------------|------|
 *
 * 2. read data
 * ___________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | read data_len byte + ack(last nack)  | stop |
 * --------|---------------------------|--------------------------------------|------|
 *
 * @param i2c_num I2C port number
 * @param reg_address slave reg address
 * @param data data to read
 * @param data_len data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static int rtc_read(uint8_t dev_address, uint8_t *data, size_t data_len)
{
    i2c_master_start();
    // write address / set cursor
    i2c_master_writeByte(dev_address << 1 | 0);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return 1;
    }
    i2c_master_writeByte(0);
    i2c_master_wait(1);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return 1;
    }
    // repeated start
    i2c_master_start();
    i2c_master_writeByte(dev_address << 1 | 1);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return 1;
    }
    for(int i=0; i < data_len-1; i++){
        data[i] = i2c_master_readByte();
        i2c_master_wait(1);
        i2c_master_send_ack();
    }
    i2c_master_wait(1);
    // last byte
    data[data_len - 1] = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return 0;
}

// query time from rtc
int rtc_get_time(rtc_time_t* t){
    uint8_t buf[3];
    uint8_t mask_4_6 = 0x70;
    uint8_t mask_0_3 = 0x0F;
    uint8_t mask_4_5 = 0x30;
    //TODO: error handeling
    i2c_master_init();
    int err = rtc_read(RTC_DEVICE_ADDRESS, &(buf[0]), 5);
    if(err){
        printf("error occured during i2c read\n");
        return err;
    }
    //get second
    // printf("%x, %x, %x\n", buf[0], buf[1], buf[2]);
    t->second[0] = (buf[0] & mask_4_6) >> 4;
    t->second[1] = (buf[0] & mask_0_3);
    // get minute
    t->minute[0] = (buf[1] & mask_4_6) >> 4;
    t->minute[1] = (buf[1] & mask_0_3);
    // get hour
    t->hour[0] = (buf[2] & mask_4_6) >> 4;
    t->hour[1] = (buf[2] & mask_4_5);
    return 0;
}

//set write time into rtc
// int rtc_set_time(rtc_time_t* time){
//     return 1;
// }
