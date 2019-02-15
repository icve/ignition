
#ifndef RTC_IO_H
#define RTC_IO_H
/*
 -1 in any field indicates not set
 year range: 0-99
 year[0]: 10 year
 year[1]: year
 example: year 19, year[0] = 1, year[1] = 9
 month 01, month[0] = 0, month[1] = 1
 is24: 1 if 24 hour clock (range 0-23)
 isAM: 1 if am, pm otherwise
*/
struct rtc_time_t{
    char year[2];
    char month[2];
    char date[2];
    char hour[2];
    char minute[2];
    char second[2];
    char is24;
    char isAM;
};

typedef struct rtc_time_t rtc_time_t;

int rtc_get_time(rtc_time_t* time);
int rtc_set_time(rtc_time_t* time);

#endif
