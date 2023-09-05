/*
    Main.cpp

    Created on: July 31, 2023

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

ADC &test_adc = adc[0];

int main(void)
{
    printf_io.turn_off_bl_enable();
    printf("\rRestart..");
	
    test_adc.readAsync(1000ms, [](int16_t val)
    {
        printf("\rADC: %d   ", val);
    });	


	for(;;)
	{
		this_thread::sleep_for(500ms);
		printf("\rV = %"PRId32" mV\n", test_adc.get_voltage());
	}

    return 0;
}
