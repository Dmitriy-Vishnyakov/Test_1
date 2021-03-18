

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../ecg_sensor.h"
#include "ads129x.h"
#include "ecg_pin_config.h"

#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
static uint8_t m_tx_buf[SPI_BUFFER_LENGTH];
static uint8_t m_rx_buf[SPI_BUFFER_LENGTH];
static uint8_t bytesToSend;

//static uint8_t adc_registers[ADC_RESULT_LENGTH];
static int32_t ecgResult[ADS_BUFFER_LENGTH][ADS_CHANNEL_COUNT];
static uint16_t historyIndexHead;
static uint16_t historyIndexTail;
static bool initPinsDone = false;
static SpiContext_t spiContext;

static int count = 0;
static volatile int checkSpi = 1;


////////////////////////////////////////////////////////////////////////////////

// prototipes
void spi_event_handler(nrf_drv_spi_evt_t const* p_event, void* p_context);
void interruptFromReadyPin(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

////////////////////////////////////////////////////////////////////////////////


void writeSpi(uint8_t length) {

	spi_xfer_done = false;
	spiContext = DO_NOTHING;
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, length, m_rx_buf, length));
	while ( !spi_xfer_done );
}
////////////////////////////////////////////////////////////////////////////////


void debugAdsChip(void) {


/*	
	int okChip = checkAdsChipId();
	if ( !okChip ) {
		return;
	}
*/
	
	
	if ( 1 ) {
		EcgParams_t ecgParams = {
			.cannelCount = 3,
			.samplingRate = 500,
			.ecgMode = INTERNAL_TEST_MODE,
		};
//		startConversion(&ecgParams);
		while(1) {
			startConversion(&ecgParams);
//			TEST_ON;
			nrf_delay_ms(5000);
			stopConversion();
//			TEST_OFF;
			nrf_delay_ms(5000);
		}	
	} else { // test ID
		if ( !initPinsDone ) pinsInit();
		chipOn();

		while(1) {
			TEST_INV;
	
	m_tx_buf[0] = READ_REG_COMMAND  | 0; // 0 is ChipID register address 
	m_tx_buf[1] = 0;  // register count to read
	m_tx_buf[2] = 0;  // get result
	writeSpi(3);
			
//			checkChipId();
			nrf_delay_us(50);
		}	
	}
}
////////////////////////////////////////////////////////////////////////////////


const AdsRerister_t regilarSettings[] = {
	{0x01, 0x05}, // CONFIG1 = default + 0x5	fMOD = 500SPS
	{0x02, 0x00}, // 0x01 for test mode
	{0x03, (char)( BIT7 | BIT6 | BIT3 | BIT2 ) }, // CONFIG3
	{0x04, (char)( BIT7 | BIT6 | BIT1 | BIT0 ) }, // Address = 04h  LOFF: Lead-Off Control Register

	{0x05, 0x00}, // Address = 05h to 0Ch CHnSET: Individual Channel Settings (n = 1 : 8)
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},

	{0x0d, 0x03}, // Address = 0Dh RLD_SENSP Right led driver positive
	{0x0e, 0x01}, // Address = 0Eh  RLD_SENSN negative
	
	{0x0f, 0xff},
	{0x10, 0xff},
	{0x11, 0xff},

	{0x15, 0x00}, // Address = 15h PACE: PACE Detect Register
	{0x16, 0x20}, // Address = 16h RESP: Respiration Control Register
	{0x17, (char)(BIT1)}, // Address = 17h CONFIG4: bit1 = Lead-off comparators enabled
	{0x18, 0x08}, // Address = 18h WCT1: Wilson Central Terminal and Augmented Lead Control Register
	{0x19, 0xd1}, // Address = 19h WCT2: Wilson Central Terminal Control Register
};
////////////////////////////////////////////////////////////////////////////////


const AdsRerister_t internalTestSettings[] = {
	{0x01, 0x05}, // CONFIG1 = default + 0x5	fMOD = 500SPS
	{0x02, 0x10}, // 0x10 for test mode
	{0x03, (char)( BIT7 | BIT6 | BIT3 | BIT2 ) }, // CONFIG3
	{0x04, (char)( BIT7 | BIT6 | BIT1 | BIT0 ) }, // Address = 04h  LOFF: Lead-Off Control Register

	{0x05, 0x05}, // Address = 05h to 0Ch CHnSET: Individual Channel Settings (n = 1 : 8)
	{0x06, 0x05},
	{0x07, 0x05},
	{0x08, 0x05},
	{0x09, 0x05},
	{0x0a, 0x05},
	{0x0b, 0x05},
	{0x0c, 0x05},

	{0x0d, 0x03}, // Address = 0Dh RLD_SENSP Right led driver positive
	{0x0e, 0x01}, // Address = 0Eh  RLD_SENSN negative
	
	{0x0f, 0xff},
	{0x10, 0xff},
	{0x11, 0xff},

	{0x15, 0x00}, // Address = 15h PACE: PACE Detect Register
	{0x16, 0x20}, // Address = 16h RESP: Respiration Control Register
	{0x17, (char)(BIT1)}, // Address = 17h CONFIG4: bit1 = Lead-off comparators enabled
	{0x18, 0x08}, // Address = 18h WCT1: Wilson Central Terminal and Augmented Lead Control Register
	{0x19, 0xd1}, // Address = 19h WCT2: Wilson Central Terminal Control Register
};
////////////////////////////////////////////////////////////////////////////////

bool checkAdsChipId(void) {

	if ( !initPinsDone ) pinsInit();
	chipOn();

	bool ret = false;
	m_tx_buf[0] = READ_REG_COMMAND  | 0; // 0 is ChipID register address 
	m_tx_buf[1] = 0;  // register count to read
	m_tx_buf[2] = 0;  // get result
	writeSpi(3);
	
	unsigned char id = m_rx_buf[2];
	if ( id == ADC1298_DEVICE_ID || id == ADC1298R_DEVICE_ID ) {
		ret = true;
	}
	if ( id == ADC1294_DEVICE_ID || id == ADC1294R_DEVICE_ID ) {
		ret = true;
	}
	
	chipOff();
	return ret;
}
////////////////////////////////////////////////////////////////////////////////


void startConversion(EcgParams_t* _ecgParams) {
	
	if ( !initPinsDone ) {
		pinsInit();
	}
	
	chipOn();
	
	if ( _ecgParams != NULL ) {
	
		EcgParams_t ecgParams = {
			.cannelCount = 3,
			.samplingRate = 500,
			.ecgMode = INTERNAL_TEST_MODE,
		};

		chipInit(&ecgParams);
//		chipInit(_ecgParams);
	}
	
	AdsRerister_t reg = {0x00, 0x01}; // fix me or comment me 
	adsSetRegister(&reg);

	spiContext = PARCE_ADS_RESULT;
	historyIndexHead = historyIndexTail = 0;
	nrf_drv_gpiote_in_event_enable(ADS_READY, true);  // ENABLE INTERRUPT;
}
////////////////////////////////////////////////////////////////////////////////


void stopConversion(void) {


	nrf_drv_gpiote_in_event_disable(ADS_READY); //  DISNABLE INTERRUPT;
//	AdsRerister_t reg = {0x00, 0x00};
//	adsSetRegister(&reg);

	chipOff();
}
////////////////////////////////////////////////////////////////////////////////


bool getEcgVector(int32_t* p_vector) {

	if ( historyIndexHead == historyIndexTail ) {
		return false;
	}
	
	memcpy(p_vector, &ecgResult[historyIndexTail][0], sizeof(int32_t)*ADS_CHANNEL_COUNT);
	historyIndexTail = (historyIndexTail+1) & (ADS_BUFFER_LENGTH-1);
	
	return true;
}
////////////////////////////////////////////////////////////////////////////////


uint8_t sendByte(uint8_t data) {

	m_tx_buf[0] = data;
	writeSpi(1);
	return m_rx_buf[0];
}
////////////////////////////////////////////////////////////////////////////////


void chipOn(void) {

//	ADS_POWER_ON;
	ADS_PWDN_OFF;
	nrf_delay_ms(250); // t por at least 130ms 2^18 slk = 500ns

	// send RESET command to ads chip
	sendByte(RESET_COMMAND);
	nrf_delay_ms(100);
	sendByte(SDATAC_COMMAND);
}
////////////////////////////////////////////////////////////////////////////////


void chipOff(void) {

	ADS_PWDN_ON;
//	ADS_POWER_OFF;
}
////////////////////////////////////////////////////////////////////////////////


void pinsInit(void) {

	nrf_gpio_cfg_output(ADS_PWDN);
	
	memset(m_tx_buf, 0, sizeof(m_tx_buf));
	memset(m_rx_buf, 0, sizeof(m_rx_buf));

	// ads ready pin init 
	ret_code_t err_code;
	err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
	err_code = nrf_drv_gpiote_in_init(ADS_READY, &in_config, interruptFromReadyPin);
	APP_ERROR_CHECK(err_code);
	
	// !!! should be nrf_drv_gpiote_in_event_disable
	nrf_drv_gpiote_in_event_enable(ADS_READY, false); //  DISNABLE INTERRUPT;
	
	// init spi
    nrf_drv_spi_config_t spi_config = SPI_ADS1298_CONFIG;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, &spiContext));
	
	initPinsDone = true;
}
////////////////////////////////////////////////////////////////////////////////


void chipInit(EcgParams_t* _ecgParams) {
	
	// set ads registers
	uint8_t regCount;
	AdsRerister_t* p_registers;
	
	switch (_ecgParams->ecgMode) {
	case ECG_MODE:
		regCount = sizeof(regilarSettings) / sizeof(AdsRerister_t);
		p_registers = (AdsRerister_t*)&regilarSettings[0];
		break;
	case INTERNAL_TEST_MODE:
		regCount = sizeof(internalTestSettings) / sizeof(AdsRerister_t);
		p_registers = (AdsRerister_t*)&internalTestSettings[0];
		break;
	default:
		regCount = 0;
		break;
	}

	for ( int i = 0; i < regCount; i++ ) {
		adsSetRegister(p_registers++);
	}

	// Addirional settings
	if ( _ecgParams->samplingRate != DEFAULT_ADC_RATE ) {
		AdsRerister_t regRate = {1,0x05};
		switch (_ecgParams->samplingRate) {
			case 250: regRate.data = 0x06; break;
			case 1000: regRate.data = 0x04; break;
			default: regRate.data = 0x05; break;
		}
		adsSetRegister(&regRate);
	}
}
////////////////////////////////////////////////////////////////////////////////


void adsSetRegister(AdsRerister_t* p_register) {

	m_tx_buf[0] = WRITE_REG_COMMAND | p_register->address; // 0 is ChipID register address 
	m_tx_buf[1] = 0;                 // register count
	m_tx_buf[2] = p_register->data;  // get result
	writeSpi(3);
}
////////////////////////////////////////////////////////////////////////////////


void parseConversionResult() {

	unsigned char ctemp;
	char* p_result = (char*)(&ecgResult[historyIndexHead][0]);
	char* reg = (char*)&m_rx_buf[4]; // skip status info and + 1

	for (int i = 0; i < ADS_CHANNEL_COUNT; i++, reg+=3 ) {
		*(p_result++) = reg[2];
		*(p_result++) = reg[1];

		ctemp = *(p_result++) = reg[0];
		if ( (ctemp & 0x80) != 0 ) {
			*p_result++ = 0xff;
		} else {
			*p_result++ = 0;
		}
	}

	historyIndexHead = (historyIndexHead+1) & (ADS_BUFFER_LENGTH-1);
}
////////////////////////////////////////////////////////////////////////////////


void interruptFromReadyPin(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {

	if ( !initPinsDone ) return;

	if ( spi_xfer_done ) {
		
		m_tx_buf[0] = RDATA_COMMAND;
		m_tx_buf[1] = 0;
		m_tx_buf[2] = 0;
		m_tx_buf[28] = RDATAC_COMMAND;
		bytesToSend = 29;
		
//		TEST2_ON;
		spi_xfer_done = false;
		spiContext = PARCE_ADS_RESULT;
	TEST_INV;
		APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, bytesToSend, m_rx_buf, bytesToSend));
		count++;
	}
}
////////////////////////////////////////////////////////////////////////////////


void spi_event_handler(nrf_drv_spi_evt_t const* p_event, void* p_context) {

	spi_xfer_done = true;
//	TEST2_OFF;

	if ( *((SpiContext_t*)p_context) == PARCE_ADS_RESULT) {
//		TEST2_ON;
		parseConversionResult();
//		TEST2_OFF;
	}
}
////////////////////////////////////////////////////////////////////////////////
