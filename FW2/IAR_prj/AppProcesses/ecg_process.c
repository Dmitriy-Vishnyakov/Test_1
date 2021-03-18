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

#include "ecg_process.h"
#include "timestamp_timer.h"
#include "nrf_drv_timer.h"
#include "compressor.h"
#include "../ecg_sensor/ecg_adc.h"

const nrf_drv_timer_t TIMER_500HZ = NRF_DRV_TIMER_INSTANCE(2);
static bool initTimerDone = false;
static short ecgVector[MAX_CHANNELS];
static enum { ADS1294, ADS1298 } chipId;
static uint16_t channelsOnChip[] = { 3, 8 };
static uint16_t channels;
EcgParams_t ecgParams;

#define SIMULATOR
#ifdef  SIMULATOR
#include "simulator_ecg.h"
uint16_t simulatorIndex = 0;

static void getEcgFromSimulator(){
	uint16_t index = simulatorIndex << 3;
	if ( simulatorIndex >= SIMULATOR_ECG_LENGTH ) {
		// qrs pause
		index = 0;
	}
	
	for ( uint16_t i = 0; i < channels; i++ ) {
		ecgVector[i] = simulatorEcgSignal[index++]<<1;
	}
	simulatorIndex++;
	if ( simulatorIndex > SIMULATOR_ECG_PERION ) {
		simulatorIndex = 0;
	}
}
#endif // SIMULATOR
////////////////////////////////////////////////////////////////////////////////


static void timeoutTimerHandler(nrf_timer_event_t event_type, void* p_context) {

	memset(ecgVector, 0, sizeof(ecgVector));
#ifdef SIMULATOR
	
	while ( adcGetEcgVector(ecgVector) ) {
		compressorAddVector(ecgVector);
	}
	
//	getEcgFromSimulator();
//	compressorAddVector(ecgVector);
#endif // SIMULATOR
	
	
#ifndef SIMULATOR
	while ( adcGetEcgVector(ecgVector) ) {
		compressorAddVector(ecgVector);
	}
#endif // not SIMULATOR

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
	err_code = nrfx_timer_init(&TIMER_500HZ, &timer_cfg, timeoutTimerHandler);
	APP_ERROR_CHECK(err_code);

	uint32_t timeTicks = 2500; //nrf_drv_timer_ms_to_ticks(&TIMEOUT_TIMER_100MS, timeMs);
	
	nrf_drv_timer_extended_compare(
		 &TIMER_500HZ, NRF_TIMER_CC_CHANNEL0, timeTicks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
}
////////////////////////////////////////////////////////////////////////////////


void ecgProcessInit() {
	
	chipId = ADS1294;
	channels = channelsOnChip[chipId];
	ecgParams.cannelCount = channelsOnChip[chipId];
	ecgParams.samplingRate = 500;
	ecgParams.ecgMode = SIMULATOR_MODE;

	adcInit(&ecgParams);
	compressorInit(channels);
}
////////////////////////////////////////////////////////////////////////////////


void sensorProcessStart() {
	
	ecgProcessInit();

	if ( !initTimerDone ) {
		initTimeoutTimer();
		initTimerDone = true;
	}
	nrf_drv_timer_clear(&TIMER_500HZ);
	nrf_drv_timer_enable(&TIMER_500HZ);
	
	adcStart();
}
////////////////////////////////////////////////////////////////////////////////


void sensorProcessStop() {

	nrf_drv_timer_disable(&TIMER_500HZ);
	adcStop();
}
////////////////////////////////////////////////////////////////////////////////
