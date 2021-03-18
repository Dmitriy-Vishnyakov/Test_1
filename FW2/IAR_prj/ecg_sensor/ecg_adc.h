#ifndef _ECG_ADC_H_
#define _ECG_ADC_H_

#include <stdint.h>
#include <types_porting.h>
#include "ecg_sensor.h"

	bool adcTestChip(void);
	void adcInit(EcgParams_t* _ecgParams);
	void adcStart(void);
	void adcStop(void);
	bool adcGetEcgVector(short* p_ecg);

#endif // _BEE_ADC_H_
