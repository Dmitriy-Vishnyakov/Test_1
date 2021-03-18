#include "stdafx.h"


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sdk_errors.h>
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "timestamp_timer.h"

const nrf_drv_timer_t TIMER_TS = NRF_DRV_TIMER_INSTANCE(1);

/////////////////////////////////////////////////////////////////////////////

void timestampClear() {

	nrf_drv_timer_clear(&TIMER_TS);
}
/////////////////////////////////////////////////////////////////////////////


int32_t getTimestamp() {
	
	nrf_drv_timer_capture(&TIMER_TS, NRF_TIMER_CC_CHANNEL1);
	int32_t current_tic = TIMER_TS.p_reg->CC[NRF_TIMER_CC_CHANNEL1];

	return current_tic;
}
/////////////////////////////////////////////////////////////////////////////


static void dummyHandler(nrf_timer_event_t event_type, void* p_context) {
}
/////////////////////////////////////////////////////////////////////////////


void timestampTimerInit() {
	
	//Configure TIMER for generating timdestamps
	nrf_drv_timer_config_t timer_cfg = //NRF_DRV_TIMER_DEFAULT_CONFIG;
	{
		.frequency          = (nrf_timer_frequency_t)4, // 1 MHz
		.mode               = (nrf_timer_mode_t)NRFX_TIMER_DEFAULT_CONFIG_MODE,
		.bit_width          = (nrf_timer_bit_width_t)3, // 32 bit
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context          = NULL
	};

	uint32_t err_code = NRF_SUCCESS;
	err_code = nrfx_timer_init(&TIMER_TS, &timer_cfg, dummyHandler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_timer_extended_compare(
		 &TIMER_TS, NRF_TIMER_CC_CHANNEL0, 0xffffffff, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	TIMER_TS.p_reg->CC[NRF_TIMER_CC_CHANNEL1] = 0;

	nrf_drv_timer_enable(&TIMER_TS);
}
/////////////////////////////////////////////////////////////////////////////
