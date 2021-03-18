#include "stdafx.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sdk_errors.h>

#include "ble_wrapper.h"


static char m_rx_buffer_fifo[RX_BUFFER_SIZE];
static uint16_t in_index = 0;
static uint16_t out_index = 0;
static uint16_t buffer_length = 0;

uint16_t BleGetRxCount() {

	return buffer_length;
}
/////////////////////////////////////////////////////////////////////////////


uint16_t BleRead(char* data, uint16_t length) {
	
	uint16_t count = 0;

	return count;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t UsbWrite(char* data, uint16_t length) {
	
	ret_code_t ret = 0;//app_usbd_cdc_acm_write(&m_app_cdc_acm, data, length);

	return ret;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t UsbInit() {

    ret_code_t ret = 0;//app_usbd_init(&usbd_config);
	
	return ret;
}
/////////////////////////////////////////////////////////////////////////////

