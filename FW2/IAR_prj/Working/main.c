/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "boards.h"
#include "bsp.h"
#include "bsp_cli.h"
#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "usb_wrapper.h"
#include "flash_mem.h"
#include "host_interfase.h"
#include "bee_data_types.h"

char version_name[] = "V0.000(RF)";
char firmvare_version[VERSION_NAME_LENGTH];

/**
 * @brief CLI interface over UART
 */
NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
            "uart_cli:~$ ",
            &m_cli_uart_transport.transport,
            '\r',
            4);

/**@file
 * @defgroup usbd_cdc_acm_example main.c
 * @{
 * @ingroup usbd_cdc_acm_example
 * @brief USBD CDC ACM example
 *
 */

//#define LED_USB_RESUME      (BSP_BOARD_LED_0)
//#define LED_CDC_ACM_OPEN    (BSP_BOARD_LED_1)
//#define LED_CDC_ACM_RX      (BSP_BOARD_LED_2)
//#define LED_CDC_ACM_TX      (BSP_BOARD_LED_3)

#define BTN_CDC_DATA_SEND       0
#define BTN_CDC_NOTIFY_SEND     1

#define BTN_CDC_DATA_KEY_RELEASE        (bsp_event_t)(BSP_EVENT_KEY_LAST + 1)

static char tx_buffer[64];
static bool m_send_flag = 0;
/////////////////////////////////////////////////////////////////////////////


/*
	key press handler
*/
static void bsp_event_callback(bsp_event_t ev)
{
//    ret_code_t ret;
    switch ((unsigned int)ev)
    {
        case CONCAT_2(BSP_EVENT_KEY_, BTN_CDC_DATA_SEND):
        {
            m_send_flag = 1;
            break;
        }
        
        case BTN_CDC_DATA_KEY_RELEASE :
        {
            m_send_flag = 0;
            break;
        }

        case CONCAT_2(BSP_EVENT_KEY_, BTN_CDC_NOTIFY_SEND):
        {
//            ret = app_usbd_cdc_acm_serial_state_notify(&m_app_cdc_acm,
//                                                       APP_USBD_CDC_ACM_SERIAL_STATE_BREAK,
//                                                       false);
//            UNUSED_VARIABLE(ret);
            break;
        }

        default:
            return; // no implementation needed
    }
}
/////////////////////////////////////////////////////////////////////////////


static void init_bsp(void)
{
    ret_code_t ret;
    ret = bsp_init(BSP_INIT_BUTTONS, bsp_event_callback);
    APP_ERROR_CHECK(ret);
    
    UNUSED_RETURN_VALUE(bsp_event_to_button_action_assign(BTN_CDC_DATA_SEND,
                                                          BSP_BUTTON_ACTION_RELEASE,
                                                          BTN_CDC_DATA_KEY_RELEASE));
    
    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);
}
/////////////////////////////////////////////////////////////////////////////


static void init_cli(void)
{
    ret_code_t ret;
    ret = bsp_cli_init(bsp_event_callback);
    APP_ERROR_CHECK(ret);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = TX_PIN_NUMBER;
    uart_config.pselrxd = RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    ret = nrf_cli_init(&m_cli_uart, &uart_config, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(&m_cli_uart);
    APP_ERROR_CHECK(ret);
}
/////////////////////////////////////////////////////////////////////////////

#define INCOME_BUFFER_LENGTH   32

void CheckBleIncommingData()
{

//		chanelId = BLE;

}
/////////////////////////////////////////////////////////////////////////////


void CheckUsbIncommingData()
{

	uint16_t data_length = UsbGetRxCount();
	char buffer[INCOME_BUFFER_LENGTH];
	if ( data_length ) {
		
//		chanelId = USB;
		
		NRF_LOG_INFO("Usb data_length: %d", data_length);
		while ( data_length > 0 ) {
			uint16_t n_count = data_length;
			if (n_count > INCOME_BUFFER_LENGTH ){
				n_count = INCOME_BUFFER_LENGTH;
			}
			uint16_t bytes_read = UsbRead(buffer, n_count);
			for ( int i = 0; i < bytes_read; i++ ) {
				addIncomingData(buffer[i]);
			}
			
 			data_length -= bytes_read;
		}
	}

}
/////////////////////////////////////////////////////////////////////////////


int main(void)
{
	memset(firmvare_version, 0, sizeof(firmvare_version));
	sprintf(&firmvare_version[0], "%s %s", version_name, __DATE__);

    ret_code_t ret;
	
    ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    
    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    init_bsp();
    init_cli();

    app_usbd_serial_num_generate();

	// init usb
	ret = UsbInit();
	
    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }


//	FlashMemWrite((char*)"Helllllllll0", 32, 0x3e000);
//	char data[32];
//	memset(data, 0, sizeof(data));
//	FlashMemErase(0x3e000);
//	FlashMemRead(data, 32, 0x3e000);


	hostInterfaseInit();
	while (true)
    {
        while (app_usbd_event_queue_process()) {
            /* Nothing to do */
        }
		
		CheckUsbIncommingData();
//		CheckBleIncommingData();
				
        if( m_send_flag )
        {
            static int  frame_counter;
			
			memset(tx_buffer, 0, sizeof(tx_buffer));
            size_t size = sprintf(tx_buffer, "Hello Nordic USB : %u\r\n", frame_counter);
			ret = UsbWrite(tx_buffer, size);
			
            if (ret == NRF_SUCCESS)
            {
                ++frame_counter;
            }
        }
        
        nrf_cli_process(&m_cli_uart);

        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
        /* Sleep CPU only if there was no interrupt since last loop processing */
        __WFE();
    }
}
/////////////////////////////////////////////////////////////////////////////

/** @} */
