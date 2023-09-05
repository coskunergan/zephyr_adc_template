/*
    ADC Lib

    Created on: July 23, 2023

    Author: Coskun ERGAN
*/
#pragma once
#include <stdio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <cassert>
#include <zephyr/irq.h>
#include <functional>

#include <zpp.hpp>
#include <zpp/timer.hpp>
#include <chrono>
#include <zpp/thread.hpp>
#include <zpp/fmt.hpp>

namespace device_adc
{
    using namespace zpp;
    using namespace std::chrono;
#if defined(ADC)
#undef ADC
#endif

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
 	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

    class ADC
    {
    private:
        const struct adc_dt_spec _spec_dt;
        struct adc_sequence_options _options;
        struct adc_sequence _sequence;
        struct IsrContext
        {
            struct k_work work;
            ADC *self;
            int16_t buffer;
            int16_t sample;
            std::function<void(int16_t)> done_cb;
            enum adc_action state;
        } _isrContext;

    public:
        ADC(const struct adc_dt_spec &spec_dt)
            : _spec_dt(spec_dt)
        {
            int result = 0;

            result = adc_channel_setup_dt(&_spec_dt);
            assert(result == 0);

            k_work_init(&_isrContext.work, _soft_isr);
            _isrContext.self = this;
        }

        virtual void readAsync(microseconds us, std::function<void(int16_t)> &&handler = nullptr)
        {
            _options =
            {
                .interval_us = us.count(),
                .callback = _hard_isr,
                .user_data = &_isrContext,
            };
            _sequence =
            {
                .options = &_options,
                .channels = BIT(_spec_dt.channel_cfg.channel_id),
                .buffer = &_isrContext.buffer,
                .buffer_size = sizeof(_isrContext.buffer),
                .resolution = _spec_dt.resolution,
            };
            _isrContext.done_cb = std::move(handler);
            _isrContext.state = ADC_ACTION_REPEAT;
            int res = adc_read_async(_spec_dt.dev, &_sequence, nullptr);
            assert(res == 0);
        }

        virtual void cancelRead()
        {
            _isrContext.done_cb = nullptr;
            _isrContext.state = ADC_ACTION_FINISH;
        }

        virtual int32_t get_voltage()
        {
            int32_t val_mv;
            if(_spec_dt.channel_cfg.differential)
            {
                val_mv = (int32_t)((int16_t)_isrContext.sample);
            }
            else
            {
                val_mv = (int32_t)_isrContext.sample;
            }
            adc_raw_to_millivolts_dt(&_spec_dt, &val_mv);
            return val_mv;
        }

    private:
        static void _soft_isr(struct k_work *work)
        {
            IsrContext *context = CONTAINER_OF(work, IsrContext, work);
            if(context->done_cb)
            {
                context->done_cb(context->sample);
            }
        }

        static enum adc_action _hard_isr(const struct device *,
                                         const struct adc_sequence *sequence,
                                         uint16_t sampling_index __unused)
        {
            IsrContext *context = static_cast<IsrContext *>(sequence->options->user_data);
            context->sample = context->buffer;
            k_work_submit(&context->work);
            return context->state;
        }
    };

    ADC adc[]
    {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, (adc_dt_spec)DT_SPEC_AND_COMMA)
    };
}
