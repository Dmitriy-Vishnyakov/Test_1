#ifndef _USB_WRAPPER_H_
#define	_USB_WRAPPER_H_

#include <stdint.h>
//#include "app_usbd.h"
//#include "app_usbd_cdc_acm.h"

#include "boards.h"
#include "bsp.h"


#define RX_BUFFER_SIZE          512

ret_code_t BleInit();
uint16_t BleGetRxCount();
uint16_t BleRead(char* data, uint16_t length);
ret_code_t BleWrite(char* data, uint16_t length);


#endif // _USB_WRAPPER_H_
