#ifndef _TIMESTAMP_TIMER_H_
#define	_TIMESTAMP_TIMER_H_

#include <stdint.h>
#include "nrf_drv_timer.h"


int32_t getTimestamp();
void timestampTimerInit();
void timestampClear();

#endif // _TIMESTAMP_TIMER_H_
