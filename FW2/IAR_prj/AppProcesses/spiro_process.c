/*
host interfase
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "../DsCommons/bit_operations.h"
#include "../DsCommons/net_level.h"
#include "../DsCommons/bee_data_types.h"
#include "../DsCommons/bee_command_codes.h"
#include "../DsCommons/bee_dev_info.h"

#include "spiro_process.h"
#include "timestamp_timer.h"
#include "nrf_drv_timer.h"
#include "compressor.h"

#include "spiro_pin_config.h"


#define IN_BUFFER_LENGTH   128
static __int32 input_buffer[IN_BUFFER_LENGTH];
static short in_index = 0;
static short done_index = 0;

const nrf_drv_timer_t TIMEOUT_TIMER_100MS = NRF_DRV_TIMER_INSTANCE(2);
static bool initDone = false;
////////////////////////////////////////////////////////////////////////////////


void spiroProcessResetTimeout() {

	nrf_drv_timer_clear(&TIMEOUT_TIMER_100MS);
}
////////////////////////////////////////////////////////////////////////////////


static void timeoutTimerHandler(nrf_timer_event_t event_type, void* p_context) {

	uint32_t  temp_ts = getTimestamp();
	TEST_INV;
	int count = 0;
	while ( done_index != in_index) {
		compressorAddRaw32(input_buffer[done_index]);
		done_index = (done_index+1)&(IN_BUFFER_LENGTH-1);
		count++;
	}

	if ( count == 0 ) {
		clrbit(temp_ts, 1); // bit 1 = "0" means no data
		compressorAddRaw32(temp_ts);
	}
	
	compressorPushRaw32();

	NRF_LOG_INFO("100 ms gone. data count %d TS %d", count, temp_ts);
}
////////////////////////////////////////////////////////////////////////////////


void initTimeoutTimer(){
	
	//Configure TIMER for data timeout
	nrf_drv_timer_config_t timer_cfg =
	{
		.frequency          = (nrf_timer_frequency_t)4, // 1 MHz
		.mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,
		.bit_width          = (nrf_timer_bit_width_t)3, // 32 bit
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context          = NULL
	};

	uint32_t err_code = NRF_SUCCESS;
	err_code = nrfx_timer_init(&TIMEOUT_TIMER_100MS, &timer_cfg, timeoutTimerHandler);
	APP_ERROR_CHECK(err_code);

	uint32_t timeMs = 200;
	uint32_t timeTicks = nrf_drv_timer_ms_to_ticks(&TIMEOUT_TIMER_100MS, timeMs);
	
	nrf_drv_timer_extended_compare(
		 &TIMEOUT_TIMER_100MS, NRF_TIMER_CC_CHANNEL0, timeTicks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	initDone = true;
}
////////////////////////////////////////////////////////////////////////////////


//void spiroProcessInit() {
	
//}
////////////////////////////////////////////////////////////////////////////////


int testA = 0;
int testB = 0;
void interruptFromPhaseA(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {

	uint32_t  temp_ts = getTimestamp();
	
	if ( pin == IN1 ) {
		setbit(temp_ts, 0);
		testA++;
		TEST_ON;
	}
	if ( pin == IN2 ) {
		clrbit(temp_ts, 0);
		testB++;
		TEST_OFF;
	}

	setbit(temp_ts, 1); // bit 1 = "1" means timestamp from sensor
	input_buffer[in_index++] = temp_ts;
	in_index &= (IN_BUFFER_LENGTH-1);
}
////////////////////////////////////////////////////////////////////////////////


void sensorProcessStart() {
	
	if ( !initDone ) {
		initSpiroPins(interruptFromPhaseA);
		initTimeoutTimer();

//		spiroProcessInit();
	}
	
	in_index = 0;
	done_index = 0;
	LED_IR_ON;
	LED_R_ON;
	LED_G_OFF;
	
	nrf_drv_timer_clear(&TIMEOUT_TIMER_100MS);
	nrf_drv_timer_enable(&TIMEOUT_TIMER_100MS);
	timestampClear();
	
	NRF_LOG_INFO("sensorProcessStart");
}
////////////////////////////////////////////////////////////////////////////////


void sensorProcessStop() {
	
	if ( !initDone ) { // need it in debug mode
		initTimeoutTimer();
	}

	LED_IR_OFF;
	LED_R_OFF;
	LED_G_ON;

	nrf_drv_timer_disable(&TIMEOUT_TIMER_100MS);
}
////////////////////////////////////////////////////////////////////////////////
