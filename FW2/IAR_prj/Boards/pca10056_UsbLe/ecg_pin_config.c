

#include "ecg_pin_config.h"


void initEcgPins(nrfx_gpiote_evt_handler_t evt_handler) {
	
//    ret_code_t err_code;
/*
	// phase A,B
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);
	
    nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

	// phase A
    err_code = nrf_drv_gpiote_in_init(IN1, &in_config, evt_handler);
	APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(IN1, true);

	// phase B
    err_code = nrf_drv_gpiote_in_init(IN2, &in_config, evt_handler);
	APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(IN2, true);
*/	
}
////////////////////////////////////////////////////////////////////////////////
