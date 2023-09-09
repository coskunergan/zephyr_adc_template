/*
    Main.cpp

    Created on: Sep 9, 2023

    Author: Coskun ERGAN
*/

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>

#include <zpp.hpp>
#include <chrono>
#include "printf_io.h"
#include "adc_io.h"

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

using namespace zpp;
using namespace std::chrono;
using namespace device_printf;
using namespace device_adc;

typedef enum
{
    adc_volt = 0,
    adc_rms,
    adc_raw,
} adc_t;

ADC adc;

uint32_t val_rms;
uint32_t rms_sum;
#define RMS_SAMPLE_NUM 256

int main(void)
{
    printf_io.turn_off_bl_enable();

    printf("\rRestart..");

    adc.readAsync(100us, [&](size_t idx, int16_t val)
    {
        switch((adc_t)idx)
        {
            case adc_volt:
                break;
            case adc_rms:
                rms_sum -= rms_sum / RMS_SAMPLE_NUM;
                rms_sum += val * val;                
                break;
            case adc_raw:
                break;
            default:
                break;
        }        
    });

    for(;;)
    {        
        this_thread::sleep_for(500ms);        
        printf("\rVolt = %"PRId32"   \n", adc.get_voltage(adc_volt));
        val_rms = (val_rms == 0) ? 1 : val_rms;
        val_rms = (val_rms + ((rms_sum / RMS_SAMPLE_NUM) / val_rms)) / 2;
        printf("Rms = %"PRId32"   \n",  val_rms);
        printf("Raw = %"PRId32"   \n", adc.get_value(adc_raw));
    }

    return 0;
}
