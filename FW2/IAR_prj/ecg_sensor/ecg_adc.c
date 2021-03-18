
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "ecg_sensor.h"
#include "ecg_adc.h"
#include "filter_dc_remove.h"
#include "filter_ac.h"

#ifdef ADS129X
#include "./ads129x/ads129x.h"
#endif // ADS129X

#ifdef ADS1293
#include "./ads129x/ads129x.h"
#endif // ADS1293

static EcgParams_t* p_ecgParams = NULL;
////////////////////////////////////////////////////////////////////////////////


bool adcTestChip(void) {

//	debugAdsChip(); // endless loop delme
	
	return checkAdsChipId();
}
////////////////////////////////////////////////////////////////////////////////


void adcInit(EcgParams_t* _ecgParams) {

	// store ecg params
	p_ecgParams = _ecgParams;
	FiltersDCRemoveInit(ADS_CHANNEL_COUNT);
	FilterAcRemoveInit(ADS_CHANNEL_COUNT, AC_REMOVE_MODE_50, p_ecgParams->samplingRate);
}
////////////////////////////////////////////////////////////////////////////////


void adcStart(void) {

	if ( p_ecgParams == NULL ) return;
	
	startConversion(p_ecgParams);
}
////////////////////////////////////////////////////////////////////////////////


void adcStop(void) {
	
	stopConversion();
}
////////////////////////////////////////////////////////////////////////////////


bool adcGetEcgVector(short* pVector) {
	
	int32_t vector32[ADS_CHANNEL_COUNT];

	if ( !getEcgVector(&vector32[0]) ) {
		return false;
	}
	
	// filtering 
	for (int i = 0; i < ADS_CHANNEL_COUNT; i++ ) {
		pVector[i] = FiltersDCRemoveGetNext(i, vector32[i]);
		FilterAcRemoveGetNext(pVector);
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////

