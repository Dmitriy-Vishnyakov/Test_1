/*
Copyright (C) 2013
*/

#ifndef _FILTER_AC_H_
#define _FILTER_AC_H_

#define AC_REMOVE_MODE_NONE     0
#define AC_REMOVE_MODE_50       1
#define AC_REMOVE_MODE_60       2

#ifndef PI
#define PI ((double)3.1415926535897932384626433832795)
#endif // PI

#define CENTRAL_50   49.9
#define CENTRAL_60   59.9
#define DELTA_AC50   0.8
#define DELTA_AC60   0.8

/*
cos(2 * pi * 50Hz / sampling_rate) table:
        400Hz          500Hz          1000Hz
48,7    0.721397724    0.818510842    0.953548856
49,9    0.708216629    0.809754988    0.95125049
50,2    0.704881854    0.807537175    0.950667443

59,1    0.599163343
59,9    0.589055328
60,1    0.586513727
*/

// AC remove filter defines
#define  FIXED_POINT_FLOAT_15(var)  (0x8000*(float)var)

#define COS_HIGHT_VALUE (short)(FIXED_POINT_FLOAT_15(0.721397724))
#define COS_INITIAL_VAL (short)(FIXED_POINT_FLOAT_15(0.708216629))
#define COS_LOW_VALUE   (short)(FIXED_POINT_FLOAT_15(0.704881854))

#define COS_HIGHT_50   (short)(FIXED_POINT_FLOAT_15(0.721397724))
#define COS_INITIAL_50 (short)(FIXED_POINT_FLOAT_15(0.708216629))
#define COS_LOW_50     (short)(FIXED_POINT_FLOAT_15(0.704881854))

#define COS_HIGHT_60   (short)(FIXED_POINT_FLOAT_15(0.599163343))
#define COS_INITIAL_60 (short)(FIXED_POINT_FLOAT_15(0.589055328))
#define COS_LOW_60     (short)(FIXED_POINT_FLOAT_15(0.586513727))

#define  MSP_HARDWARE_MULTIPLY(res, op1, op2)  { res = (short) ( (((op1)<<1) * (op2))>>16 ); }

/*
#ifdef _MSC_VER
#define  MSP_HARDWARE_MULTIPLY(res, op1, op2)  { res = (short) ( (((op1)<<1) * (op2))>>16 ); }

#elif defined (__GNUC__)
#define  MSP_HARDWARE_MULTIPLY(res, op1, op2)  { res = (short) ( (((op1)<<1) * (op2))>>16 ); }

#elif defined (__IAR_SYSTEMS_ICC__)
#include "msp430.h"
#define  MSP_HARDWARE_MULTIPLY(res,op1,op2)  { MPYS = (op1<<1); OP2 = op2; res = RESHI; }

#else
#error "MSP_HARDWARE_MULTIPLY does not have a default include file"
#endif
*/

typedef struct {
    short ac_noise_prev0;
	short ac_noise_prev1;
    short input_prev;
	short cos;
	short power_hz_change_indicator;
} ac_remove_filter_st;

void FilterAcRemoveInit(short channel_count, short filter_id, double sampling_rade);
void FilterAcRemoveGetNext(short* ecg_vector);

#endif // _FILTER_AC_H_
