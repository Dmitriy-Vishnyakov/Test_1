#ifndef _USB_WRAPPER_H_
#define	_USB_WRAPPER_H_

#include <stdint.h>
#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"

#include "boards.h"
#include "bsp.h"

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define RX_BUFFER_SIZE          512

ret_code_t UsbInit();
uint16_t UsbGetRxCount();
uint16_t UsbRead(char* data, uint16_t length);
ret_code_t UsbWrite(char* data, uint16_t length);


#endif // _USB_WRAPPER_H_
