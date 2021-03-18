/*
Beecardia firmware.
*/

#include "stdafx.h"

#include <string.h>
#include "types_porting.h"
#include "filter_dc_remove.h"
#include "bit_operations.h"

static short init_countdown;
static short low_tau_count[MAX_CHANNEL_COUNT];
static __int32 capacitor[MAX_CHANNEL_COUNT];
/////////////////////////////////////////////////////////////////////////////

/*
Filter_Capasitor_Init set the middle (24 bit adc dynamic range middle) 
*/
void FiltersDCRemoveInit(short channel_count)
{
	init_countdown = INIT_VECTORS_COUNT;
	
	memset(capacitor, 0, sizeof(capacitor));
	memset(low_tau_count, 0, sizeof(low_tau_count));
}
/////////////////////////////////////////////////////////////////////////////

/*
Filter_Capasitor is like a split capacitor in classic ECG
it remove baseline.
average[i] = ( average[i-1] * alpha + input[i] * betta );
*/
short FiltersDCRemoveGetNext(short channel, __int32 input_value) {
	
	__int32 ret_value;
	__int64 low_pass_value;

	if ( init_countdown ) {
		init_countdown--;
		low_pass_value  = ( (__int64)capacitor[channel] + (__int64)input_value ) >> 1;
		capacitor[channel] = (__int32)low_pass_value;
		ret_value = 0;
	} else {
		unsigned __int32 betta;
		unsigned __int32 alpha;

		if ( low_tau_count[channel] ) {
			low_tau_count[channel]--;
			betta = MAX_BETTA_VALUE;
		} else {
			betta = MIN_BETTA_VALUE;
		}
		alpha = 0xffffffff - betta + 1;
		
		low_pass_value  = (__int64)capacitor[channel]*alpha + (__int64)input_value*betta;
		capacitor[channel] = *(((__int32*)&low_pass_value)+1);
		ret_value  = ( input_value - capacitor[channel] ) >> NOISY_BITS;
		
		if ( (ret_value > LOW_TAU_LEVEL ) || ( ret_value < -LOW_TAU_LEVEL) ) {
			low_tau_count[channel] = LOW_TAU_COUNT;
			low_pass_value = capacitor[channel] = input_value;
			ret_value = 0;
		}
	}

	if ( ret_value > MAX_ECG_VALUE) ret_value = MAX_ECG_VALUE;
	if ( ret_value < MIN_ECG_VALUE) ret_value = MIN_ECG_VALUE;

	return (short)(ret_value);
}
/////////////////////////////////////////////////////////////////////////////
