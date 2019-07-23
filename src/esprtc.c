#include "rtc_io.h"
#include "c_types.h"
#include "esp_system.h"
struct esprtc_t{
    rtc_time_t t;
    uint32 last_rtc_cycle_count;
};

void reset_rtc_time_t(struct esprtc_t* et){
    et->last_rtc_cycle_count = system_get_rtc_time();
    et->t.second[0] = 0;
    et->t.second[1] = 0;
    et->t.minute[0] = 0;
    et->t.minute[1] = 0;
    et->t.hour[0] = 0;
    et->t.hour[1] = 0;
    et->t.is24 = 1;
}

/*
* given output_base, add op1 and op2 and compute the output and carry in that base
* for examle, add_in_base(1, 1, 2, &o, &c) -> o == 0, c == 1 
* for examle, add_in_base(20, 5, 10, &o, &c) -> o == 5, c == 2 
 */
inline void add_in_base(int op1, int op2, int output_base, int *val_output, int *carry_output){
    *val_output = (op1 + op2) % output_base;
    *carry_output = (op1 + op2) / output_base;
}

/*
 * update value in esp_rtc_clock
 *  
 */
void esprtc_update(struct esprtc_t *et){
    const uint32_t current_cycle_count = system_get_rtc_time();
    const uint32_t cycle_diff =  current_cycle_count - et->last_rtc_cycle_count;
    const unsigned int second_passed = cycle_diff * system_rtc_clock_cali_proc() / 1000000;
    esprtc_update_helper(et, second_passed);
    et->last_rtc_cycle_count = current_cycle_count;
}

/*
 * helper function for incrementing the values in esprtc_t by seconds passed
 * calling directly is not recommended
 */
void esprtc_update_helper(struct esprtc_t *et, unsigned int second_passed){
    if(second_passed > 0){
        int carry_output;
        int val_output;
        add_in_base(et->t.second[1], second_passed, 10, &val_output, &carry_output);
        et->t.second[1] = (char) val_output;
        add_in_base(et->t.second[0], carry_output, 6, &val_output, &carry_output);
        et->t.second[0] = (char) val_output;
        add_in_base(et->t.minute[1], carry_output, 10, &val_output, &carry_output);
        et->t.minute[1] = (char) val_output;
        add_in_base(et->t.minute[0], carry_output, 6, &val_output, &carry_output);
        et->t.minute[0] = (char) val_output;

        int hour = et->t.hour[0] * 10 + et->t.hour[1] + carry_output;
        hour = hour % 24;
        et->t.hour[1] = (char) (hour % 10);
        et->t.hour[0] = (char) (val_output / 10 % 10);

}