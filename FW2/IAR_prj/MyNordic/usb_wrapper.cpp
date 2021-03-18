#include "stdafx.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sdk_errors.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "usb_wrapper.h"


static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

/**
 * @brief CDC_ACM class instance
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

#define READ_SIZE 1
static char m_rx_char[READ_SIZE];
static char m_rx_buffer_fifo[RX_BUFFER_SIZE];
static uint16_t in_index = 0;
static uint16_t out_index = 0;
static uint16_t buffer_length = 0;

static void usbd_user_ev_handler(app_usbd_event_type_t event);
static const app_usbd_config_t usbd_config = {
	.ev_state_proc = usbd_user_ev_handler
};
/////////////////////////////////////////////////////////////////////////////


/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t (headphones)
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
//            bsp_board_led_on(LED_CDC_ACM_OPEN);

            /*Setup first transfer*/
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_rx_char,
                                                   READ_SIZE);
            UNUSED_VARIABLE(ret);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
			
//            bsp_board_led_off(LED_CDC_ACM_OPEN);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
			
//            bsp_board_led_invert(LED_CDC_ACM_TX);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            NRF_LOG_INFO("Bytes waiting: %d", app_usbd_cdc_acm_bytes_stored(p_cdc_acm));
            do
            {
                /*Get amount of data transfered*/
                size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                NRF_LOG_INFO("RX: size: %lu char: %c", size, m_rx_char[0]);
				m_rx_buffer_fifo[in_index++] = m_rx_char[0];
				in_index &= (RX_BUFFER_SIZE-1);
				buffer_length++;

                /* Fetch data until internal buffer is empty */
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                            m_rx_char,
                                            READ_SIZE);
            } while (ret == NRF_SUCCESS);

//            bsp_board_led_invert(LED_CDC_ACM_RX);
            break;
        }
        default:
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////


static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
//            bsp_board_led_off(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_DRV_RESUME:
//            bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
//            bsp_board_leds_off();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled()) {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////


uint16_t UsbGetRxCount() {

	return buffer_length;
}
/////////////////////////////////////////////////////////////////////////////


uint16_t UsbRead(char* data, uint16_t length) {
	
	uint16_t count = 0;
	for (int i = 0; i < length; i++) {
		data[i] = m_rx_buffer_fifo[out_index++];
		out_index &= (RX_BUFFER_SIZE-1);
		count++;
		buffer_length--;
		
		if ( out_index == in_index ) {
			// unexpected case
			buffer_length = 0;
			break;
		}
	}
	return count;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t UsbWrite(char* data, uint16_t length) {
	
	ret_code_t ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, data, length);

	return ret;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t UsbInit() {

    ret_code_t ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("USBD CDC ACM example started.");

    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

	in_index = 0;
	out_index = 0;
	buffer_length = 0;

	
	return ret;
}
/////////////////////////////////////////////////////////////////////////////

