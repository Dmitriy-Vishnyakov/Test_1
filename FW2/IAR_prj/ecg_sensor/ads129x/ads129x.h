
#ifndef _BEE_ADS1294_H_
#define _BEE_ADS1294H_

#include <stdint.h>
#include <types_porting.h>
#include <ecg_pin_config.h>

#define NOISY_BITS              6
#define	ADC_RESULT_LENGTH       27

#define ADC1294_DEVICE_ID       0x90
#define ADC1294R_DEVICE_ID      0xD0
#define ADC1298_DEVICE_ID       0x92
#define ADC1298R_DEVICE_ID      0xD2

#define WAKEUP_COMMAND          0x02 // Wake-up from standby mode
#define STANDBY_COMMAND         0x04 // Enter standby mode
#define RESET_COMMAND           0x06 // Reset the device
#define START_COMMAND           0x08 // Start/restart (synchronize) conversion
#define STOP_COMMAND            0x0a // Stop conversion
#define READ_REG_COMMAND        0x20 // Read n nnnn registers starting at address r rrrr
#define WRITE_REG_COMMAND       0x40 // Wte n nnnn registers starting at address r rrrr
#define RDATAC_COMMAND          0x10 // Enable Read Data Continuous mode
#define SDATAC_COMMAND          0x11 // Stop Read Data Continuously mod
#define RDATA_COMMAND           0x12 // Read data by command; supports multiple read back.

#define DEFAULT_ADC_RATE        500
#define ADS_BUFFER_LENGTH       32
#define ADS_CHANNEL_COUNT       8

#define SPI_BUFFER_LENGTH       32


/*
#define ADS_POWER            NRF_GPIO_PIN_MAP(0,13)
#define ADS_POWER_ON         nrf_gpio_pin_write(ADS_POWER, 1);
#define ADS_POWER_OFF        nrf_gpio_pin_write(ADS_POWER, 0);
*/
#define ADS_PWDN            NRF_GPIO_PIN_MAP(1,13)
#define ADS_PWDN_OFF        nrf_gpio_pin_write(ADS_PWDN, 1);
#define ADS_PWDN_ON         nrf_gpio_pin_write(ADS_PWDN, 0);
#define ADS_READY           NRF_GPIO_PIN_MAP(0,2)

#define SPI_SS_PIN   255
#define SPI_MISO_PIN 5
#define SPI_MOSI_PIN 43
#define SPI_SCK_PIN  41

#define SPI_ADS1298_CONFIG                                   \
{                                                            \
    .sck_pin      = SPI_SCK_PIN,                             \
    .mosi_pin     = SPI_MOSI_PIN,                            \
    .miso_pin     = SPI_MISO_PIN,                            \
    .ss_pin       = SPI_SS_PIN,                              \
    .irq_priority = SPI_DEFAULT_CONFIG_IRQ_PRIORITY,         \
    .orc          = 0xFF,                                    \
    .frequency    = NRF_SPI_FREQ_1M,                         \
    .mode         = NRF_DRV_SPI_MODE_1,                      \
    .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,         \
}
////////////////////////////////////////////////////////////////////////////////


typedef enum {
    DO_NOTHING,
    PARCE_ADS_RESULT,
} SpiContext_t;


typedef struct {
	uint8_t address;
	uint8_t data;
} AdsRerister_t;

	bool checkAdsChipId(void);
	void startConversion(EcgParams_t* _ecgParams);
	void stopConversion(void);
	
	void pinsInit(void);
	
	void chipOff(void);
	void chipOn(void);
	void chipInit(EcgParams_t* _ecgParams);
	
	uint8_t adsReadRegister(AdsRerister_t* p_register);
	void adsSetRegister(AdsRerister_t* p_register); // return verified registers data
	uint8_t adsSendByte(uint8_t data);
	bool getEcgVector(int32_t* p_vector);
	void readConversionResult();
	void parseConversionResult();
	void debugAdsChip();
	
//	void AdsGetEcg(short* p_ecg);
//	void setControlRegisters(ecg_parameters_st* p_ecg_parameters);
	

#endif // _BEE_ADS1294_H_
