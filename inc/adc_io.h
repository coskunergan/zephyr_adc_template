/*
    ADC Lib

    Created on: Sep 9, 2023

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
#include <chrono>
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

    static const struct adc_dt_spec adc_channels[] =
    {
        DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
    };
    class ADC
    {
    private:
        struct adc_sequence_options options;
        struct adc_sequence sequence;
        const size_t channel_count = ARRAY_SIZE(adc_channels);
        size_t channel_index;
        struct IsrContext
        {
            struct k_work work;
            int16_t buffer;
            int16_t sample[ARRAY_SIZE(adc_channels)];
            std::function<void(size_t, int16_t)> done_cb;
            enum adc_action state;
            ADC *self;
        } isrContext;

    public:
        ADC()
        {
            int err;
            for(size_t i = 0U; i < channel_count; i++)
            {
                err = adc_channel_setup_dt(&adc_channels[i]);
                if(err < 0)
                {
                    printk("Could not setup channel #%d (%d)\n", i, err);
                    return;
                }
            }
            k_work_init(&isrContext.work, _soft_isr);
        }

        virtual void readAsync(microseconds us, std::function<void(size_t, int16_t)> &&handler = nullptr)
        {
            options =
            {
                .interval_us = (uint32_t)us.count(),
                .callback = _hard_isr,
                .user_data = &isrContext,
            };
            sequence =
            {
                .options = &options,
                .channels = BIT(adc_channels[0].channel_cfg.channel_id),
                .buffer = &isrContext.buffer,
                .buffer_size = sizeof(isrContext.buffer),
                .resolution = adc_channels[0].resolution,
            };
            isrContext.self = this;
            channel_index = 0;
            isrContext.done_cb = std::move(handler);
            isrContext.state = ADC_ACTION_CONTINUE;
            int res = adc_read_async(adc_channels[0].dev, &sequence, nullptr);
            assert(res == 0);
        }

        virtual void cancelRead()
        {
            isrContext.state = ADC_ACTION_FINISH;
        }

        virtual int32_t get_voltage(size_t idx)
        {
            int32_t val_mv;
            if(adc_channels[idx].channel_cfg.differential)
            {
                val_mv = (int32_t)((int16_t)isrContext.sample[idx]);
            }
            else
            {
                val_mv = (int32_t)isrContext.sample[idx];
            }
            adc_raw_to_millivolts_dt(&adc_channels[idx], &val_mv);
            return val_mv;
        }

        virtual int32_t get_value(size_t idx)
        {
            int32_t val;
            if(adc_channels[idx].channel_cfg.differential)
            {
                val = (int32_t)((int16_t)isrContext.sample[idx]);
            }
            else
            {
                val = (int32_t)isrContext.sample[idx];
            }
            return val;
        }

    private:
        static void _soft_isr(struct k_work *work)
        {
            IsrContext *context = CONTAINER_OF(work, IsrContext, work);
            if(context->self->channel_count > 1)
            {
                context->self->sequence.channels = BIT(adc_channels[context->self->channel_index].channel_cfg.channel_id);
                context->self->sequence.resolution = adc_channels[context->self->channel_index].resolution;
                int res = adc_read_async(adc_channels[context->self->channel_index].dev, &context->self->sequence, nullptr);
                assert(res == 0);
            }
            if(context->done_cb)
            {
                if(context->self->channel_index == 0)
                {
                    context->done_cb(context->self->channel_count - 1, context->sample[context->self->channel_count - 1]);
                }
                else
                {
                    context->done_cb(context->self->channel_index - 1, context->sample[context->self->channel_index - 1]);
                }
            }            
        }

        static enum adc_action _hard_isr(const struct device *,
                                         const struct adc_sequence *seq,
                                         uint16_t sampling_index __unused)
        {
            IsrContext *context = static_cast<IsrContext *>(seq->options->user_data);
            context->sample[context->self->channel_index] = context->buffer;
            if(context->state != ADC_ACTION_FINISH)
            {
                if(context->self->channel_count == 1)
                {
                    context->state = ADC_ACTION_REPEAT;
                    k_work_submit(&context->work);
                }
                else if(++context->self->channel_index < context->self->channel_count)
                {
                    context->state = ADC_ACTION_CONTINUE;
                    k_work_submit(&context->work);
                }
                else
                {
                    if(context->state != ADC_ACTION_REPEAT)
                    {
                        context->state = ADC_ACTION_REPEAT;
                        context->self->channel_index--;
                    }
                    else
                    {
                        context->state = ADC_ACTION_CONTINUE;
                        context->self->channel_index = 0;
                        k_work_submit(&context->work);
                    }
                }
            }
            return context->state;
        }
    };
}
