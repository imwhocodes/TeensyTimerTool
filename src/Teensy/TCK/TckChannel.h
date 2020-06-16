#pragma once

#include "../../ITimerChannel.h"
#include "ErrorHandling/error_codes.h"
#include "core_pins.h"

namespace TeensyTimerTool
{
    class TCK_t;

#if defined(ARDUINO_TEENSYLC) // quick hack for T-LC, should be improved later (using systick?)

    class TckChannel : public ITimerChannel
    {

        protected:
            uint32_t startCNT, currentPeriod, nextPeriod;
            callback_t callback;
            bool triggered;
            bool periodic;

            bool block = false;
            friend TCK_t;


            inline void tick(){
                static bool lock = false;

                if (!lock && this->currentPeriod != 0 && this->triggered && (micros() - this->startCNT) >= this->currentPeriod)
                {
                    lock = true;
                    this->startCNT = micros();
                    this->triggered = this->periodic; // i.e., stays triggerd if periodic, stops if oneShot
                    callback();
                    this->currentPeriod = this->nextPeriod;
                    lock = false;
                }
            }


        public:
        
            inline TckChannel() { triggered = false; }
            inline virtual ~TckChannel(){};

            inline errorCode begin(callback_t cb, uint32_t period, bool periodic)
            {
                this->triggered = false;
                this->periodic = periodic;
                this->currentPeriod = period;
                this->nextPeriod = period;
                this->callback = cb;

                this->startCNT = micros();

                return errorCode::OK;
            }

            inline void start()
            {
                this->startCNT = micros();
                this->triggered = true;
            }

            inline errorCode stop()
            {
                this->triggered = false;
                return errorCode::OK;
            }

            inline void setCurrentPeriod(uint32_t microSeconds)
            {
                this->currentPeriod = microSeconds;
            }

            inline void setNextPeriod(uint32_t microSeconds)
            {
                this->nextPeriod = microSeconds;
            }

            inline uint32_t getCurrentPeriod(void){
                return this->currentPeriod;
            }

            inline uint32_t getNextPeriod(void){
                return this->nextPeriod;
            }


            inline errorCode trigger(uint32_t delay) // µs
            {
                this->startCNT = micros();
                this->currentPeriod = delay;
                this->nextPeriod = delay;
                this->triggered = true;
                return errorCode::OK;
            }

    };

#else

    class TckChannel : public ITimerChannel
    {

        protected:
            uint32_t startCNT, currentPeriod, nextPeriod;
            callback_t callback;
            bool triggered;
            bool periodic;

            bool block = false;

            friend TCK_t;

            static inline uint32_t microsecondToCPUCycles(uint32_t microSecond){
                return microSecond * (F_CPU / 1'000'000);
            }

            static inline uint32_t CPUCyclesToMicroseond(uint32_t cpuCycles){
                return (1'000'000.0f / F_CPU) * cpuCycles;
            }


            void tick()
            {
                static bool lock = false;

                if (!lock && this->currentPeriod != 0 && this->triggered && (ARM_DWT_CYCCNT - this->startCNT) >= this->currentPeriod)
                {
                    lock = true;
                    this->startCNT = ARM_DWT_CYCCNT;
                    this->triggered = this->periodic; // i.e., stays triggerd if periodic, stops if oneShot
                    callback();
                    lock = false;
                }
            }


        public:
            inline TckChannel() { triggered = false; }
            inline virtual ~TckChannel(){};

            errorCode begin(callback_t cb, uint32_t period, bool periodic)
            {
                this->triggered = false;
                this->periodic = periodic;
                this->currentPeriod = microsecondToCPUCycles(period);

                this->nextPeriod = this->currentPeriod;
                this->callback = cb;

                this->startCNT = ARM_DWT_CYCCNT;

                return errorCode::OK;
            }

            void start()
            {
                this->startCNT = ARM_DWT_CYCCNT;
                this->triggered = true;
            }

            errorCode stop()
            {
                this->triggered = false;
                return errorCode::OK;
            }

            inline errorCode trigger(uint32_t delay) // µs
            {
                this->startCNT = ARM_DWT_CYCCNT;
                this->nextPeriod = microsecondToCPUCycles(delay);
                this->currentPeriod = this->nextPeriod - 68; //??? compensating for cycles spent computing?

                this->triggered = true;

                return errorCode::OK;
            }

            inline float getMaxPeriod()
            {
                return CPUCyclesToMicroseond(0xFFFF'FFFF);
            }

            inline errorCode setCurrentPeriod(uint32_t microSeconds)
            {
                this->currentPeriod = microsecondToCPUCycles(microSeconds);

                return errorCode::OK;
            }

            inline errorCode setNextPeriod(uint32_t microSeconds)
            {
                this->nextPeriod = microsecondToCPUCycles(microSeconds);

                return errorCode::OK;
            }


            uint32_t getCurrentPeriod()
            {
                return CPUCyclesToMicroseond(this->currentPeriod);
            }

            uint32_t getNextPeriod()
            {
                return CPUCyclesToMicroseond(this->nextPeriod);
            }

    };

#endif

} // namespace TeensyTimerTool
