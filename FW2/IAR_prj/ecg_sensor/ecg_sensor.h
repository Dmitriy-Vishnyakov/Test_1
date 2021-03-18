#ifndef _ECG_SENSOR_H_
#define _ECG_SENSOR_H_

#include <stdint.h>


#define ADS129X

#define ECG_MODE             0
#define INTERNAL_TEST_MODE   1
#define SIMULATOR_MODE       2

typedef struct {
	uint16_t cannelCount;
	uint16_t samplingRate;
	uint16_t ecgMode;
} EcgParams_t;


#endif // _BEE_SENSOR_H_
