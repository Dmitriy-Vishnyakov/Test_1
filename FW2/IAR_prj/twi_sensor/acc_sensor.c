
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "acc_sensor.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "nrf_drv_twi.h"
#include "ecg_pin_config.h"

////////////////////////////////////////////////////////////////////////////////
/* TWI instance ID. */
#define TWI_INSTANCE_ID     1

/* Common addresses definition for acc. */
#define LM75B_ADDR            0x19U // заводской адрес LIS3DH
#define LM75B_REG_TEMP        0x00U
#define LM75B_REG_CONF        0x01U
#define LM75B_REG_THYST       0x02U
#define LM75B_REG_TOS         0x03U

#define LIS3DH_CTRL_REG1      0x20U
#define LIS3DH_CTRL_REG2      0x21U
#define LIS3DH_CTRL_REG3      0x22U
#define LIS3DH_CTRL_REG4      0x23U
#define LIS3DH_CTRL_REG5      0x24U
#define LIS3DH_CTRL_REG6      0x25U

#define LIS3DH_INT1_CFG       0x30U
#define LIS3DH_INT1_SRC       0x31U
#define LIS3DH_INT1_THS       0x32U
#define LIS3DH_INT1_DURATION  0x33U

#define LIS3DH_INT2_CFG       0x34U
#define LIS3DH_INT2_SRC       0x35U
#define LIS3DH_INT2_THS       0x36U
#define LIS3DH_INT2_DURATION  0x37U

// reg 20 
#define Z_EN        0x04
#define Y_EN        0x02
#define X_EN        0x01
#define LP_MODE     0x08
#define ODR_50      0x40
#define ODR_100     0x50
#define ODR_200     0x60
#define ODR_400     0x70

// reg 21
// By default

// reg 22  LIS3DH_CTRL_REG3
#define I1_CLICK   0x80

// reg 23 
#define S2G    0x00
#define S4G    0x10
#define S8G    0x20
#define S16G   0x30


// reg 38
#define ZDOUDLE       0x20
#define ZSINGLE       0x10
#define XSINGLE       0x01

// reg 39
#define CLICK_SRC_Z       0x04
#define CLICK_SRC_SINGL   0x10
#define CLICK_SRC_DOUDLE  0x20
#define CLICK_SRC_IA      0x40

#define REG_COUNT   10
const uint8_t LIS3DH_register_settings[REG_COUNT][2]={

 {LIS3DH_CTRL_REG1, 0x77}, // включить преобразование x y z ODR = 400 Hz
 {LIS3DH_CTRL_REG2, 0x00}, // High-pass filter disabled
 {LIS3DH_CTRL_REG3, 0x80},// Interrupt activity 1 driven to INT1 pad
 {LIS3DH_CTRL_REG4, 0x10}, // // FS = ±2 g
 {0x38, 0x01}, // CLICK_CFG   tap singl
 {0x39, 0x01},      // CLICK_SRC
 {0x3a, 0x08}, // Duration = 0
 {0x3b, 0xaa},// TIME_LIMIT
 {0x3c, 0xa0}, //TIME_LATENCY
 {0x3d, 0x08}, //TIME WINDOW
	
/*	
	{LIS3DH_CTRL_REG1, ODR_400 | X_EN}, // включить преобразование x y z ODR = 400 Hz
	{LIS3DH_CTRL_REG2, 0x00}, // High-pass filter disabled
	{LIS3DH_CTRL_REG3, I1_CLICK},// Interrupt activity 1 driven to INT1 pad
	{LIS3DH_CTRL_REG4, S4G}, // // FS = ±2 g
	{LIS3DH_CTRL_REG5, 0x8}, //

 	{0x38, XSINGLE},                                   // CLICK_CFG   tap singl
	{0x39, CLICK_SRC_Z|CLICK_SRC_SINGL|CLICK_SRC_IA},  // CLICK_SRC
	{0x3a, 0xa0},             // CLICK_THS  = 0
	{0x3b, 0xa0},             // TIME_LIMIT
	{0x3c, 0x08},             // TIME_LATENCY tics of ODR(10ms per tic)
	{0x3d, 0x08},             // TIME WINDOW
 
*/ 
};

static uint8_t m_sample;
static uint8_t registr[2];
//static uint8_t rxData[2];

const nrf_drv_twi_xfer_desc_t twiTransfer = {
	.type     = NRF_DRV_TWI_XFER_TXRX,
	.address  = LM75B_ADDR,
	.primary_length = 1,
	.secondary_length = 1,
	.p_primary_buf = registr,
	.p_secondary_buf = (uint8_t *)&m_sample
};

/* Mode for LM75B. */
#define NORMAL_MODE 0U

/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);


/**
 * @brief Function for setting active mode on MMA7660 accelerometer.
 */
void LM75B_set_mode(void)
{
    ret_code_t err_code;

    /* Writing to LM75B_REG_CONF "0" set temperature sensor in NORMAL mode. */
    uint8_t reg[2] = {LM75B_REG_CONF, NORMAL_MODE};
    err_code = nrf_drv_twi_tx(&m_twi, LM75B_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    /* Writing to pointer byte. */
    reg[0] = LM75B_REG_TEMP;
    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, LM75B_ADDR, reg, 1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
}

/**
 * @brief Function for handling data from temperature sensor.
 *
 * @param[in] temp          Temperature in Celsius degrees read from sensor.
 */
__STATIC_INLINE void data_handler(uint8_t temp)
{
//	NRF_LOG_INFO("Temperature: %d Celsius degrees.", temp);
}

/**
 * @brief TWI events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	TEST_OFF;

    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                data_handler(m_sample);
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}
////////////////////////////////////////////////////////////////////////////////


void interruptFromAccPin(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {


	TEST_ON;

	registr[0] = 0x31;
	nrf_drv_twi_xfer(&m_twi, &twiTransfer, false);
}
////////////////////////////////////////////////////////////////////////////////


/**
 * @brief UART initialization.
 */
void twi_init (void)
{
	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_lm75b_config = {
	   .scl                = NRF_GPIO_PIN_MAP(0,24),
	   .sda                = NRF_GPIO_PIN_MAP(0,13),
	   .frequency          = NRF_DRV_TWI_FREQ_100K,
	   .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
	   .clear_bus_init     = true
	};

	err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&m_twi);
	
	
	// ACC_INT_PIN init 
	err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
	err_code = nrf_drv_gpiote_in_init(ACC_INT_PIN, &in_config, interruptFromAccPin);
	APP_ERROR_CHECK(err_code);
	nrf_drv_gpiote_in_event_disable(ACC_INT_PIN); //  DISNABLE INTERRUPT;

}

/**
 * @brief Function for reading data from temperature sensor.
 */
/*
static void read_sensor_data()
{
    m_xfer_done = false;

    // Read 1 byte from the specified address - skip 3 bits dedicated for fractional part of temperature.
    ret_code_t err_code = nrf_drv_twi_rx(&m_twi, LM75B_ADDR, rxData, 1);
    APP_ERROR_CHECK(err_code);
}
*/

////////////////////////////////////////////////////////////////////////////////
//volatile uint32_t pin;

void writeTwi(uint8_t * data, uint8_t len) {

	m_xfer_done = false;
	nrf_drv_twi_tx(&m_twi, LM75B_ADDR, data, len, false);
	while (!m_xfer_done);

}


void accDebug() {
	
	twi_init();

	// init acc settings (registers)
    for( int i = 0; i < REG_COUNT; i++ ) {
		
      registr[0] = (LIS3DH_register_settings[i][0]);
      registr[1] = (LIS3DH_register_settings[i][1]);
	  
	  writeTwi(registr, 2);
	  
//      nrf_drv_twi_tx(&m_twi, LM75B_ADDR, registr, 2, false); //отдаем адрес регистра
      //registr[0] = (LIS3DH_register_settings[i][1]);
      //nrf_drv_twi_tx(&m_twi, LM75B_ADDR,registr, 1, false); //отдаем значение регистра
//      nrf_delay_ms(20);
    }
//    registr[0] = LIS3DH_CTRL_REG1;
//    nrf_drv_twi_tx(&m_twi, LM75B_ADDR, registr, 1, false);
//	nrf_delay_ms(10);
//    nrf_drv_twi_rx(&m_twi, LM75B_ADDR, rxData, 2);  
//	nrf_delay_ms(10);
	
	
	nrf_drv_gpiote_in_event_enable(ACC_INT_PIN, true);  // ENABLE INTERRUPT;

	
	while(1) {
		
		nrf_delay_ms(100); 
	}

	
}
////////////////////////////////////////////////////////////////////////////////


bool accCheckChip(void) {

//	debugAdsChip(); // endless loop delme
	
	return true;
}
////////////////////////////////////////////////////////////////////////////////


void accInit() {
	twi_init();
}
////////////////////////////////////////////////////////////////////////////////

