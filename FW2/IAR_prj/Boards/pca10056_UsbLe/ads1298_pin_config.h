#ifndef _ASD1298_PIN_CONFIG_H_
#define _ASD1298_PIN_CONFIG_H_

#include "nrf_gpio.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"

//#define LED_R          NRF_GPIO_PIN_MAP(0,29)


#define ADS129X_CS_ON    ;
#define ADS129X_CS_OFF   ;
#define CLEAR_PENDING_INTERRUPTS  ;
#define ADS129X_ENABLE_INTERRUPT  ;
#define ADS129X_DISNABLE_INTERRUPT ;
// fix me and rename me please
// #define START_ADS129X       {setbit(ADS129X_START_OUT, ADS129X_PIN_START);}
// #define STOP_ADS129X        {clrbit(ADS129X_START_OUT, ADS129X_PIN_START);}

#define ADS129X_WAIT_TXDONE  ;
#define ADS129X_WAIT_RXDATA  ;
#define ADS129X_TXBUF        dummyByte


#define START_ADS129X     ;
#define STOP_ADS129X      ;

#define ADS129X_RXBUF     0


#endif // _ASD1298_PIN_CONFIG_H_
