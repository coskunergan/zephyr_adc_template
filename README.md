# ZephyrRTOS Non-Block Task ADC Template C++20 

![alt text](doc/img_logo.png)

# Introduction

This repositories has been prepared to be an example of the effective use of ADC hardware with ZephyrRTOS. Based on C++20, it allows ADC sampling of one or more channels with the help of DMA without task blocking.

# Why

In embedded processors, writing code in a superloop of CPU cores reduces functionality. For this reason, it is possible to write a more effective library by taking advantage of RTOS features. In this example, it is object to manage multiple ADC channels with a ring buffer stack driver access method to the ADC implementation using the Kernel Timer features of the hardware ISR and ZephyrRTOS device module without using Sperloop and to present the data to the user interface.

The interval between ADC cycles for this process can be entered parametrically in microseconds. In addition, reading data can be processed by calling a lambda closure obj or static C function in each ADC cycle done. I think the example main.cpp function will be explanatory on this subject.

As a result, you can access and easily read the data from within the application while ADC cycles are run in the background in real time.

# Installation

Must install the ZPP module under the zephyr folder before compiling.

Zephyr started guide installation and more see:
https://docs.zephyrproject.org/latest/develop/getting_started/index.html

Zephyr RTOS repositories see:
https://github.com/zephyrproject-rtos/zephyr

ZPP Module installation:
https://github.com/lowlander/zpp

For a helpful setup and test utility see:
https://github.com/lowlander/zpp_bootstrap




