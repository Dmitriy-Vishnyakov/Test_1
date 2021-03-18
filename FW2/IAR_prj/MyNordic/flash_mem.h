#ifndef _FLASH_MEM_H_
#define	_FLASH_MEM_H_

#include <stdint.h>
#include "nrf_fstorage.h"


#define FLASH_BLOCK0ADR        0x3e000

#define FLASH_DEVICEID_OFFSET  0x0
#define FLASH_VERSION_OFFSET   0x100

#define READ_BUFFER_LRNGTH     (0x100*4)

ret_code_t flashMemRead(char* data, uint16_t length, uint32_t block_adr);
ret_code_t flashMemWrite(char* data, uint16_t length, uint32_t block_adr);
ret_code_t flashMemErase(uint32_t block_adr);

ret_code_t flashMemSegmentRead(char* data, uint16_t length, uint32_t offset);
ret_code_t flashMemSegmentWrite(char* data, uint16_t length, uint32_t offset);


#endif // _FLASH_MEM_H_
