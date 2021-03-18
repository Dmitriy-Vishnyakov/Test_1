#ifndef _ACC_SENSOR_H_
#define _ACC_SENSOR_H_

#include <stdint.h>
#include <types_porting.h>


#define ACC_INT_PIN           NRF_GPIO_PIN_MAP(0,10)


	bool accCheckChip(void);
	void accInit();
	void accDebug();

#endif // _ACC_SENSOR_H_
