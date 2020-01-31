#pragma once

#include "../../ITimerChannel.h"
#include "GPTmap.h"
#include "core_pins.h"

namespace TeensyTimerTool
{
    class GptChannel : public ITimerChannel
    {
     public:
        inline GptChannel(IMXRT_GPT_t*, callback_t*);
        inline virtual ~GptChannel();

        inline errorCode begin(callback_t cb, unsigned tcnt, bool periodic);
        inline void trigger(uint32_t);
        inline void setPeriod(uint32_t) {}

        bool isPeriodic;

     protected:
        IMXRT_GPT_t* regs;
        uint32_t reload;
    };

    // IMPLEMENTATION ==============================================

    GptChannel::GptChannel(IMXRT_GPT_t* registers, callback_t* cbStorage)
        : ITimerChannel(cbStorage), regs(registers)
    {
    }

    errorCode GptChannel::begin(callback_t cb, unsigned micros, bool periodic)
    {
        isPeriodic = periodic;
        setCallback(cb);
        if (isPeriodic)
        {
            uint32_t pid_clock_mhz = (CCM_CSCMR1 & CCM_CSCMR1_PERCLK_CLK_SEL) ? 24 : (F_BUS_ACTUAL / 1000000);
            double tmp = micros * (pid_clock_mhz / 1.0);
            uint32_t reload = tmp > 0xFFFF'FFFF ? 0xFFFF'FFFF : (uint32_t)tmp;
            regs->SR = 0x3F;         // clear all interupt flags
            regs->IR = GPT_IR_OF1IE; // enable OF1 interrupt
            regs->OCR1 = reload - 1; // set overflow value
            regs->CR |= GPT_CR_EN;   // enable timer
        }
        return errorCode::OK;
    }

    GptChannel::~GptChannel()
    {
        regs->CR &= ~GPT_CR_EN;
        regs->IR = 0x00;
        setCallback(nullptr);
    }

    void GptChannel::trigger(uint32_t micros) //should be optimized somehow
    {
        uint32_t pid_clock_mhz = (CCM_CSCMR1 & CCM_CSCMR1_PERCLK_CLK_SEL) ? 24 : (F_BUS_ACTUAL / 1000000);
        double tmp = micros * (pid_clock_mhz / 1.0);
        uint32_t reload = tmp > 0xFFFF'FFFF ? 0xFFFF'FFFF : (uint32_t)tmp;

        regs->SR = 0x3F;         // clear all interupt flags
        regs->IR = GPT_IR_OF1IE; // enable OF1 interrupt
        regs->OCR1 = reload - 1; // set overflow value
        regs->CR |= GPT_CR_EN;   // enable timer
    }

} // namespace TeensyTimerTool