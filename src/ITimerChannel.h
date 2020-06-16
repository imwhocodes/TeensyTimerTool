#pragma once

#include "types.h"

namespace TeensyTimerTool
{
    class ITimerChannel
    {

        protected:
            inline ITimerChannel(callback_t* cbStorage = nullptr);
            callback_t* pCallback;

        public:
            virtual errorCode begin(callback_t callback, uint32_t period, bool oneShot) = 0;
            virtual errorCode begin(callback_t callback, float period, bool oneShot) { return postError(errorCode::wrongType); };
            
            virtual errorCode trigger(uint32_t delay) = 0;
            virtual errorCode trigger(float delay) { return postError(errorCode::wrongType); }

            virtual float getMaxPeriod(void) const { postError(errorCode::notImplemented); return 0;};

            virtual errorCode setPeriod(uint32_t microSeconds) { return postError(errorCode::notImplemented); }
            virtual errorCode setCurrentPeriod(uint32_t microSeconds) { return postError(errorCode::notImplemented); }
            virtual errorCode setNextPeriod(uint32_t microSeconds) { return postError(errorCode::notImplemented); }

            virtual uint32_t getCurrentPeriod(void) const { return 0; }
            virtual uint32_t getNextPeriod(void) const { return 0; }

            virtual void start(void){};
            virtual errorCode stop(void) { return errorCode::OK; }
            inline void setCallback(callback_t);


    };

    // IMPLEMENTATION ====================================================

    ITimerChannel::ITimerChannel(callback_t* cbStorage)
    {
        this->pCallback = cbStorage;
    }

    void ITimerChannel::setCallback(callback_t cb)
    {
        *pCallback = cb;
    }

} // namespace TeensyTimerTool