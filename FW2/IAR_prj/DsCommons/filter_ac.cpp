/*
  Copyright (C) 2013 - 2014
*/


#ifdef __IAR_SYSTEMS_ICC__
#include "bit_operations.h"
//#include "contiki.h"
#endif

#include <string.h>
#include <math.h>
#include "filter_ac.h"
#include "types_porting.h"
/////////////////////////////////////////////////////////////////////////////

static short ac_channel_count = 3;
static short ac_filter_id = AC_REMOVE_MODE_NONE;
static short cos_hight;
static short cos_low;
static ac_remove_filter_st	theA50[MAX_CHANNEL_COUNT];
/////////////////////////////////////////////////////////////////////////////


void FilterAcRemoveInit(short channel_count, short filter_id, double sampling_rade)
{
	short j;
	short cos_initial;
	
	double cos_value;
	double initial_ac_value;
	double delta_ac_value;

	ac_filter_id = filter_id;
	switch (ac_filter_id) {
		case AC_REMOVE_MODE_50:
			initial_ac_value = CENTRAL_50;
			delta_ac_value = DELTA_AC50;
			break;
		case AC_REMOVE_MODE_60:
			initial_ac_value = CENTRAL_60;
			delta_ac_value = DELTA_AC60;
			break;
		default: 
			initial_ac_value = CENTRAL_50;
			delta_ac_value = DELTA_AC50;
			break;
	}
	
	cos_value = cos(2 * PI * (initial_ac_value - delta_ac_value) / sampling_rade);
	cos_hight = (short)(0x8000*cos_value);
	
	cos_value = cos(2 * PI * initial_ac_value / sampling_rade);
	cos_initial = (short)(0x8000*cos_value);
		
	cos_value = cos(2 * PI * (initial_ac_value + delta_ac_value/2) / sampling_rade);
	cos_low = (short)(0x8000*cos_value);
		
	ac_channel_count = channel_count;
	memset( &theA50, 0, sizeof(theA50) );
	for ( j = 0; j < ac_channel_count; j++ ) {
		theA50[j].cos = cos_initial;
	}

	/*
	DEBUG results: // 400Hz, CENTRAL_50 = 49.9, CENTRAL_60 = 59.9, delta = 0.8
	MODE_50:
	hi       = 23495 (0.717032253)
	initial  = 23206 (0.708216548)
	low      = 23061 (0.703766703)
	MODE_60:
	hi       = 19633 (0.59916329)
	initial  = 19302 (0.58905524)
	low      = 19135 (0.583966314)
	*/
}
/////////////////////////////////////////////////////////////////////////////

/*
RtFilters_ACRemove_GetNext implement adaptive 50 Hz notch filtering
*/
void FilterAcRemoveGetNext(short* ecg_vector)
{
	short  j;
	short  ac_noise_estimat;
	short  f_estimate;
	short  input_value;
	ac_remove_filter_st* p_filtr;
	
	if ( ac_filter_id == AC_REMOVE_MODE_NONE ) return;
	
	for ( j = 0; j < ac_channel_count; j++ ) {
		p_filtr = &theA50[j];
		input_value = ecg_vector[j];
		
		/*		noise_estimat(nT) = 2 * N * noise(nT - T) - noise(nT - 2T);
		*		N = cos( 2 * pi * 50Hz / sampling_rate );
		*/
		MSP_HARDWARE_MULTIPLY( ac_noise_estimat, (p_filtr->ac_noise_prev0 << 1), p_filtr->cos );
		ac_noise_estimat -= p_filtr->ac_noise_prev1;
		
		/*		f_estimate = (input(nT) - noise_estimat(nT)) - (input(nT - T) - noise(nT - T));
		*		If this function is zero, our estimate is correct and future estimates will track the noise frequency.
		*		If this  function is not zero, we need an adjustment before estimating the future point
		*/
		f_estimate = ( (input_value - (ac_noise_estimat>>3)) -  (p_filtr->input_prev - (p_filtr->ac_noise_prev0>>3)) );
		if ( f_estimate > 0 ) {
			ac_noise_estimat++;
			if ( ac_noise_estimat > 0 ) {
				p_filtr->power_hz_change_indicator++;
			} else {
				p_filtr->power_hz_change_indicator--;
			}
		}
		if ( f_estimate < 0 ) {
			ac_noise_estimat--;
			if ( ac_noise_estimat < 0 ) {
				p_filtr->power_hz_change_indicator++;
			} else {
				p_filtr->power_hz_change_indicator--;
			}
		}
		
		/*		in case we move on one direction for a long time so let us change ac frequency */
		if (p_filtr->power_hz_change_indicator < -16) {
			p_filtr->cos -= 1;
			p_filtr->power_hz_change_indicator += 8;
		}
		if (p_filtr->power_hz_change_indicator > 16) {
			p_filtr->cos += 1;
			p_filtr->power_hz_change_indicator -= 8;
		}
		
		if ( p_filtr->cos > cos_hight ) p_filtr->cos = cos_hight;
		if ( p_filtr->cos < cos_low )   p_filtr->cos = cos_low;
		
		p_filtr->ac_noise_prev1 = p_filtr->ac_noise_prev0;
		p_filtr->ac_noise_prev0 = ac_noise_estimat;
		p_filtr->input_prev = input_value;
		
		ecg_vector[j] = (short)( input_value - (ac_noise_estimat>>3) );
	}
}
/////////////////////////////////////////////////////////////////////////////
