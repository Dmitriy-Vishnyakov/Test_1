#include "stdafx.h"


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sdk_errors.h>
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "flash_mem.h"

#ifdef TEST_FLASH	
	DsDeviceId dsDeviceId;
	flashMemSegmentRead((char*)&dsDeviceId,
		sizeof(dsDeviceId), FLASH_DEVICEID_OFFSET);
#endif

#define CHECK_IF_INIT if ( !initDone ) { \
		rc = InitFlashMem(); \
		initDone = true; \
	} \


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
static bool initDone = false;
static nrf_fstorage_api_t * p_fs_api;

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = 0x3e000,
    .end_addr   = 0x3ffff,
};


static ret_code_t InitFlashMem() {

    p_fs_api = &nrf_fstorage_sd;

	ret_code_t rc;
	rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);

	return rc;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t flashMemRead(char* data, uint16_t length, uint32_t block_adr) {

	ret_code_t rc;
	CHECK_IF_INIT;
	rc = nrf_fstorage_read(&fstorage, block_adr, data, length);
	
	return rc;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t  flashMemWrite(char* data, uint16_t length, uint32_t block_adr) {
	
	ret_code_t rc;
	CHECK_IF_INIT;
	rc = nrf_fstorage_write(&fstorage, block_adr, data, length, NULL);

    while (nrf_fstorage_is_busy(&fstorage));

	return rc;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t flashMemErase(uint32_t block_adr) {

	ret_code_t rc;
	CHECK_IF_INIT;
	rc = nrf_fstorage_erase(&fstorage, block_adr, 1, NULL);

    while (nrf_fstorage_is_busy(&fstorage));
	
	return rc;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t flashMemSegmentRead(char* data, uint16_t length, uint32_t offset) {
	
	if ( length + offset > READ_BUFFER_LRNGTH ) {
		return NRF_ERROR_INVALID_ADDR;
	}
	
	ret_code_t rc;
	CHECK_IF_INIT;
	char buffer[READ_BUFFER_LRNGTH];
	rc = flashMemRead(buffer, READ_BUFFER_LRNGTH, FLASH_BLOCK0ADR);
	if ( rc == NRF_SUCCESS ) {
		memcpy(data, &buffer[offset], length);
	}
	
	return rc;
}
/////////////////////////////////////////////////////////////////////////////


ret_code_t flashMemSegmentWrite(char* data, uint16_t length, uint32_t offset) {

	if ( length + offset > READ_BUFFER_LRNGTH ) {
		return NRF_ERROR_INVALID_ADDR;
	}
	
	ret_code_t rc;
	CHECK_IF_INIT;
	char buffer[READ_BUFFER_LRNGTH];
	rc = flashMemRead(buffer, READ_BUFFER_LRNGTH, FLASH_BLOCK0ADR);
	if ( rc == NRF_SUCCESS ) {
		memcpy(&buffer[offset], data, length);
		flashMemErase(FLASH_BLOCK0ADR);
		flashMemWrite(buffer, sizeof(buffer), FLASH_BLOCK0ADR);
	}

	return rc;
}
/////////////////////////////////////////////////////////////////////////////


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////
