

#ifndef _SPIRO_PIN_CONFIG_H_
#define _SPIRO_PIN_CONFIG_H_

#include "nrf_gpio.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"




#define LED_R          NRF_GPIO_PIN_MAP(0,29)
#define LED_G          NRF_GPIO_PIN_MAP(0,31)
#define LED_B          NRF_GPIO_PIN_MAP(0,3)
#define LED_IR         NRF_GPIO_PIN_MAP(0,2)

#define LED_R_ON  nrf_gpio_pin_write(LED_R, 0);
#define LED_R_OFF nrf_gpio_pin_write(LED_R, 1);

#define LED_G_ON  nrf_gpio_pin_write(LED_G, 0);
#define LED_G_OFF nrf_gpio_pin_write(LED_G, 1);

#define LED_B_ON  nrf_gpio_pin_write(LED_B, 0);
#define LED_B_OFF nrf_gpio_pin_write(LED_B, 1);

#define LED_IR_ON  nrf_gpio_pin_write(LED_IR, 1);
#define LED_IR_OFF nrf_gpio_pin_write(LED_IR, 0);

#define TEST_PIN       NRF_GPIO_PIN_MAP(0,24)
#define TEST_ON  nrf_gpio_pin_write(TEST_PIN, 1);
#define TEST_OFF nrf_gpio_pin_write(TEST_PIN, 0);
#define TEST_INV nrf_gpio_pin_toggle(TEST_PIN);

#define TEST2_PIN       NRF_GPIO_PIN_MAP(0,13)
#define TEST2_ON  nrf_gpio_pin_write(TEST2_PIN, 1);
#define TEST2_OFF nrf_gpio_pin_write(TEST2_PIN, 0);
#define TEST2_INV nrf_gpio_pin_toggle(TEST2_PIN);

#define IN1            NRF_GPIO_PIN_MAP(1,9)
#define IN2            NRF_GPIO_PIN_MAP(1,11)


//#define BUTTONS_NUMBER 1
//#define BUTTON_1       NRF_GPIO_PIN_MAP(1,13)
//#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP


void initSpiroPins(nrfx_gpiote_evt_handler_t evt_handler);

#endif // _SPIRO_PIN_CONFIG_H_
