#pragma once
#include "../../ITimerChannel.h"
#include "Arduino.h"
#include "ErrorHandling/error_codes.h"
#include "config.h"
#include "imxrt.h"

namespace TeensyTimerTool
{
    class TMRChannel : public ITimerChannel
    {
        public:
            inline TMRChannel(IMXRT_TMR_CH_t* regs, callback_t* cbStorage);
            inline virtual ~TMRChannel();

            inline errorCode begin(callback_t cb, uint32_t tcnt, bool periodic) override;
            inline errorCode begin(callback_t cb, float tcnt, bool periodic) override;

            inline errorCode trigger(uint32_t tcnt) override;
            inline errorCode trigger(float tcnt) override;

            inline float getMaxPeriod(void) override;
            inline uint32_t getCurrentPeriod(void) override;
            inline uint32_t getNextPeriod(void) override;

            inline errorCode setCurrentPeriod(uint32_t us) override;
            inline errorCode setNextPeriod(uint32_t us) override;
            
            inline void setPrescaler(uint32_t psc); // psc 0..7 -> prescaler: 1..128

            inline float_t microsecondToCounter(const float_t us) const;
            inline float_t counterToMicrosecond(const float_t cnt) const;


        protected:
            IMXRT_TMR_CH_t* regs;
            callback_t** pCallback = nullptr;
            float pscValue;
            uint32_t pscBits;
    };

    // IMPLEMENTATION ==============================================

    TMRChannel::TMRChannel(IMXRT_TMR_CH_t* regs, callback_t* cbStorage)
        : ITimerChannel(cbStorage)
    {
        this->regs = regs;
        setPrescaler(TMR_DEFAULT_PSC);
    }

    TMRChannel::~TMRChannel()
    {
    }

    errorCode TMRChannel::begin(callback_t cb, uint32_t tcnt, bool periodic)
    {
        return begin(cb, (float)tcnt, periodic);
    }

    float_t TMRChannel::microsecondToCounter(const float_t us) const {
        return us * 150.0f / pscValue;
    }

    float_t TMRChannel::counterToMicrosecond(const float_t cnt) const {
        return cnt * pscValue / 150.0f;
    }


    errorCode TMRChannel::begin(callback_t cb, float tcnt, bool periodic)
    {
        const float_t t = microsecondToCounter(tcnt);
        uint16_t reload;

        if(t > 0xFFFF)
        {
            postError(errorCode::periodOverflow);
            reload = 0xFFFE;
        }
        else{
            reload = (uint16_t)t - 1;
        }

        regs->CTRL = 0x0000;
        regs->LOAD = 0x0000;
        regs->COMP1 = reload;
        regs->CMPLD1 = reload;
        regs->CNTR = 0x0000;
        setCallback(cb);
        regs->CSCTRL &= ~TMR_CSCTRL_TCF1;
        regs->CSCTRL |= TMR_CSCTRL_TCF1EN;

        if (!periodic)
            regs->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_ONCE | TMR_CTRL_LENGTH;

        else
            regs->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_LENGTH;

        return t > 0xFFFF ? errorCode::periodOverflow : errorCode::OK;
    }

    errorCode TMRChannel::trigger(uint32_t tcnt)
    {
        return trigger((float)tcnt);
    }

    errorCode TMRChannel::trigger(float tcnt) // quick and dirty, should be optimized
    {
        const float_t t = microsecondToCounter(tcnt);

        if(t <= 0xFFFF){
            const uint16_t reload = (uint16_t)t;

            regs->CTRL = 0x0000;
            regs->LOAD = 0x0000;
            regs->COMP1 = reload;
            regs->CMPLD1 = reload;
            regs->CNTR = 0x0000;

            regs->CSCTRL &= ~TMR_CSCTRL_TCF1;
            regs->CSCTRL |= TMR_CSCTRL_TCF1EN;

            regs->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_ONCE | TMR_CTRL_LENGTH;

            return errorCode::OK;
        }
        else
        {
            return errorCode::periodOverflow;
        }
        

    }

    uint32_t TMRChannel::getCurrentPeriod(){
        return counterToMicrosecond(regs->COMP1);
    }

    uint32_t TMRChannel::getNextPeriod(){
        return counterToMicrosecond(regs->CMPLD1);
    }

    errorCode TMRChannel::setCurrentPeriod(uint32_t us){
        const float_t t = microsecondToCounter(us);

        if(t <= 0xFFFF){
            const uint16_t reload = (uint16_t)t;
            regs->COMP1 = reload;

            //Do we need to wait some cycle for IP bus to update here?

            if(regs->CNTR > reload){
                //no conter overflow, force comparison
                regs->CNTR = reload;
            }

            return errorCode::OK;
        }
        else{
            return errorCode::periodOverflow;
        }
    }

    errorCode TMRChannel::setNextPeriod(uint32_t us)
    {
        const float_t t = microsecondToCounter(us);

        if(t <= 0xFFFF){
            const uint16_t reload = (uint16_t)t;
            regs->CMPLD1 = reload;
            return errorCode::OK;
        }
        else{
            return errorCode::periodOverflow;
        }

    }

    void TMRChannel::setPrescaler(uint32_t psc) // psc 0..7 -> prescaler: 1..128
    {
        pscValue = 1 << (psc & 0b0111);
        pscBits = 0b1000 | (psc & 0b0111);
    }

    float TMRChannel::getMaxPeriod()
    {
        return counterToMicrosecond(0xFFFE);
    }

} // namespace TeensyTimerTool
