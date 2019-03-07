#include "c_types.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c_master.h"
#include "rtc_io.h"
#define RTC_DEVICE_ADDRESS 0x68
static xSemaphoreHandle RTC_IO_SEMAPHORE = NULL;
/**
 * @brief i2c master initialization
 */
int rtc_init(void)
{
    printf("rtc init\n");
    i2c_master_gpio_init();
    i2c_master_init();
    vSemaphoreCreateBinary(RTC_IO_SEMAPHORE);
    return 0;
}


// TODO: user friendly set time
int rtc_write_reg_raw(uint8_t address, uint8_t value)
{
    xSemaphoreTake(RTC_IO_SEMAPHORE, -1);
    i2c_master_start();
    i2c_master_writeByte(RTC_DEVICE_ADDRESS << 1 | 0);

    if( ! i2c_master_checkAck())
    {
        i2c_master_stop();
        xSemaphoreGive(RTC_IO_SEMAPHORE);
        return 1;
    }
    i2c_master_writeByte(address);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        xSemaphoreGive(RTC_IO_SEMAPHORE);
        return 1;
    }
    i2c_master_writeByte(value);
    i2c_master_stop();
    xSemaphoreGive(RTC_IO_SEMAPHORE);
    return 0;
}
static int rtc_read(uint8_t dev_address, uint8_t *data, size_t data_len)
{
    int errcode = 1;
    i2c_master_start();
    // write address / set cursor
    i2c_master_writeByte(dev_address << 1 | 0);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return errcode;
    }
    errcode++;
    i2c_master_writeByte(0);
    i2c_master_wait(1);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return errcode;
    }
    errcode++;
    // repeated start
    i2c_master_start();
    i2c_master_writeByte(dev_address << 1 | 1);
    if(!i2c_master_checkAck()){
        i2c_master_stop();
        return errcode;
    }
    errcode++;
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
    uint8_t buf[13];
    uint8_t mask_4_6 = 0x70;
    uint8_t mask_0_3 = 0x0F;
    uint8_t mask_4_5 = 0x30;
    //TODO: error handeling
    xSemaphoreTake(RTC_IO_SEMAPHORE, -1);
    int err = rtc_read(RTC_DEVICE_ADDRESS, &(buf[0]), sizeof buf);
    xSemaphoreGive(RTC_IO_SEMAPHORE);
    if(err){
        printf("error occured during i2c read, errcode: %d\n", err);
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
    t->hour[1] = (buf[2] & mask_0_3);
     //get date
    t->date[0] = (buf[4] & mask_4_6) >> 4;
    t->date[1] = (buf[4] & mask_0_3);
    //get mounth
    t->mounth[0] = (buf[5] & mask_4_6) >> 4;
    t->mounth[1] = (buf[5] & mask_0_3);
    //get year
    t->year[0] = (buf[6] & mask_4_6) >> 4;
    t->year[1] = (buf[6] & mask_0_3);
    return 0;
}

//set write time into rtc
// int rtc_set_time(rtc_time_t* time){
//     return 1;
// }
