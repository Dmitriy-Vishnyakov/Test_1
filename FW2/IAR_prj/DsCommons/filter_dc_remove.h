/*
Beecardia firmware.
*/

#ifndef _FITER_DC_REMOVE_H_
#define	_FITER_DC_REMOVE_H_

#include "types_porting.h"

#define MAX_ECG_VALUE         SHORT_MAX
#define MIN_ECG_VALUE         SHORT_MIN

#define MAX_BETTA_VALUE       0x800000
#define MIN_BETTA_VALUE       0x80000

// INIT_VECTORS_COUNT = Xch * XHz * Xsec
// LOW_TAU_COUNT = XHz * Xseconds
// LOW_TAU_LEVEL = XmV * Xppmv

// ADS1294_CHIP
#define NOISY_BITS            6
#define INIT_VECTORS_COUNT    3*500*2
#define LOW_TAU_COUNT         500*4
#define LOW_TAU_LEVEL         40*328

#ifdef ADS1293_CHIP
#define NOISY_BITS            4
#define INIT_VECTORS_COUNT    3*400*2
#define LOW_TAU_COUNT         400*4
#define LOW_TAU_LEVEL         40*382
#endif

#ifdef ADS1298_CHIP
#define NOISY_BITS            6
#undef INIT_VECTORS_COUNT
#define INIT_VECTORS_COUNT    8*500*2
#define LOW_TAU_COUNT         500*4
#define LOW_TAU_LEVEL         40*328
#endif

void  FiltersDCRemoveInit(short channel_count);
short FiltersDCRemoveGetNext(short channel, __int32 input_value);

#endif // _FITER_DC_REMOVE_H_
