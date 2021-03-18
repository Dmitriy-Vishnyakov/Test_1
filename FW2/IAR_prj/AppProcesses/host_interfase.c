/*
host interfase
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "../DsCommons/bit_operations.h"
#include "../DsCommons/net_level.h"
#include "../DsCommons/bee_data_types.h"
#include "../DsCommons/bee_command_codes.h"
#include "../DsCommons/bee_dev_info.h"
#include "../DsCommons/monitoring_modes.h"

#include "../MyNordic/flash_mem.h"
#include "host_interfase.h"
#include "compressor.h"

#ifdef SPIRO
#include "spiro_process.h"
#endif
#ifdef ECG
#include "ecg_process.h"
#endif
#ifdef PHONE
#include "mic_process.h"
#endif

extern char firmware_version[VERSION_NAME_LENGTH];
extern char* received_msg_buffer;
//------------------------------------------------------------------------------

#pragma data_alignment=2
static char response_buffer[128];
#pragma data_alignment=2
static char data_block_buffer[512+24];
WriteDataFn_t channelWriteFn = NULL; //callback function to push data to channel 

static DsDeviceId dsDeviceId = { DEVICE_NAME_DEFAULT, DEVICE_NUMBER_DEFAULT };
static uint8_t monitoringMode = MONITORING_MODE_OFF;

static const char *cmd_lst[] = {
	"AT+GetDeviseID\0",
	"AT+GetFwVersion\0",
	"AT+StartMonitoring\0",
	"AT+StopMonitoring\0"};
   //12345456789 123456789 123456789 32
enum { 
	AT_DEVICE_ID,
	AT_FW_VERSION,
	AT_START,
	AT_STOP };
#define STRING_MESSAGE_COUNT     4
#define STRING_MESSAGE_LENGTH    128
static char strMessageBuffer[STRING_MESSAGE_LENGTH];
static uint16_t strBufferIndex = 0;
//------------------------------------------------------------------------------

void parseIncomingMessage(char* received_msg);
void readDeviceId();
void storeDeviceId();
void onCommandSetMonitoringMode(uint8_t modeValue);
void onDataBlockReady(char* data);
void sendDataToHost(char* data);
//------------------------------------------------------------------------------


void sendDataToHost(char* data) {

	uint16_t count;
	count = ((message_header_st*)data)->data_length +
		sizeof(message_header_st) + 1;

	if ( channelWriteFn != NULL ) {
		NRF_LOG_INFO("send response with length: %d", count);
		channelWriteFn((char*)data, count);
	}
}
////////////////////////////////////////////////////////////////////////////////


void addResponseDataToQueue(Response *response) {

	uint16_t count;
	count = ((message_header_st*)response)->data_length + sizeof(message_header_st) + 1;

	if ( count > 0 && channelWriteFn != NULL ) {
		NRF_LOG_INFO("_send length: %d", count);
		channelWriteFn((char*)response, count);
	}
}
////////////////////////////////////////////////////////////////////////////////


void onDataBlockReady(char* data) {

	NetLevelCreateResponse(data_block_buffer, (char*)data, 512, CMD_DATA_BLOCK);
	addResponseDataToQueue((Response *)data_block_buffer);
}
////////////////////////////////////////////////////////////////////////////////


void handleOppCode(uint16_t opp_code, char* received_msg) {

	bool response = false;
	uint8_t tempChar;


	switch ( opp_code ) {
		
		case CMD_GET_DEVICE_ID:
			readDeviceId();
			NetLevelCreateResponse(response_buffer, (char*)&dsDeviceId,
				sizeof(dsDeviceId), CMD_DEVICE_ID);
			response = true;
		break;
		
		case CMD_SET_DEVICE_ID:
			NetLevelGetMessageData(received_msg, (char*)&dsDeviceId,
				sizeof(dsDeviceId));
			storeDeviceId();
			NetLevelCreateResponse(response_buffer, (char*)&dsDeviceId,
				sizeof(dsDeviceId), CMD_DEVICE_ID);
			response = true;
		break;
		
		case CMD_GET_FIRMWARE_VERSION:
			NetLevelCreateResponse(response_buffer, firmware_version,
				sizeof(firmware_version), CMD_FIRMWARE_VERSION);
			response = true;
		break;
		
		case CMD_SET_MONITORING_MODE:
			NetLevelGetMessageData(received_msg, (char*)&tempChar,
				sizeof(tempChar));
			onCommandSetMonitoringMode(tempChar);
			NetLevelCreateResponse(response_buffer, (char*)&monitoringMode,
				sizeof(monitoringMode), CMD_MONITORING_MODE);
			response = true;
		break;
		case CMD_MONITORING_MODE:
			NetLevelCreateResponse(response_buffer, (char*)&monitoringMode,
				sizeof(monitoringMode), CMD_MONITORING_MODE);
			response = true;
		break;
	}
	
	if ( response ) {
		addResponseDataToQueue((Response *)response_buffer);
	}
}
////////////////////////////////////////////////////////////////////////////////


void parseIncomingMessage(char* received_msg) {

	uint16_t opp_code = ((message_header_st*)received_msg)->opcode;

	NRF_LOG_INFO("Got a message: 0x%.2x", opp_code);

	if ( !(NetLevelIsCrcCorrect(received_msg)) ) {
		opp_code = -1;
	}

	handleOppCode(opp_code, received_msg);
}
////////////////////////////////////////////////////////////////////////////////


void hostInterfaseInit() {
	
	NetLevelInit();
	monitoringMode = MONITORING_MODE_OFF;
	compressorSaveDataBlock = onDataBlockReady;
	
	strBufferIndex = 0;
	memset(strMessageBuffer, 0, sizeof(strMessageBuffer));
}
////////////////////////////////////////////////////////////////////////////////


void checkStringMessage(char data) {
	
	strBufferIndex = strBufferIndex & (STRING_MESSAGE_LENGTH-1);
	strMessageBuffer[strBufferIndex++] = data;

	if ( data == 0 ) {
		strBufferIndex = 0;
		memset(strMessageBuffer, 0, sizeof(strMessageBuffer));
		return;
	}
	
	uint16_t opp_code = -1;
	for (uint16_t msgNomber = 0; msgNomber < STRING_MESSAGE_COUNT; msgNomber++ ) {
		if ( strstr((char *)strMessageBuffer, cmd_lst[msgNomber]) != NULL ) {
			
			switch ( msgNomber ) {
				case AT_DEVICE_ID: opp_code = CMD_GET_DEVICE_ID; break;
				case AT_FW_VERSION:
					opp_code = CMD_GET_FIRMWARE_VERSION; break;
				case AT_START:
					onCommandSetMonitoringMode(MONITORING_MODE_RUN);
					opp_code = CMD_MONITORING_MODE;
					break;
				case AT_STOP:
					onCommandSetMonitoringMode(MONITORING_MODE_OFF);
					opp_code = CMD_MONITORING_MODE;
					break;
			}
			handleOppCode(opp_code, response_buffer);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////


void addIncomingData(char data) {


	if ( NetLevelAddIncomingByte(data) >= 0 )	{
		parseIncomingMessage(received_msg_buffer);
		return;
	}
	
	checkStringMessage(data);
}
////////////////////////////////////////////////////////////////////////////////


void onCommandSetMonitoringMode(uint8_t modeValue) {

	if ( monitoringMode == modeValue ) {
		NRF_LOG_INFO("the same mode. ignoge it: %d", monitoringMode);
		return;
	}
	
	monitoringMode = modeValue;
	NRF_LOG_INFO("now monitoringMode will be: %d", monitoringMode);

	if ( monitoringMode == MONITORING_MODE_RUN ) {
		sensorProcessStart();
	}
	
	if ( monitoringMode == MONITORING_MODE_OFF ) {
		sensorProcessStop();
	}
}
////////////////////////////////////////////////////////////////////////////////


void readDeviceId() {
	
	flashMemSegmentRead((char*)&dsDeviceId,
		sizeof(dsDeviceId), FLASH_DEVICEID_OFFSET);
	
	if ( dsDeviceId.device_class[0] == 0xff ) {
		dsDeviceId = (DsDeviceId){DEVICE_NAME_DEFAULT, DEVICE_NUMBER_DEFAULT};
		flashMemSegmentWrite((char*)&dsDeviceId,
			sizeof(dsDeviceId), FLASH_DEVICEID_OFFSET);
	}
	
	dsDeviceId.device_class[DEVICE_ID_NAME_LENGTH-1] = 0;
	dsDeviceId.device_number[DEVICE_ID_NAME_LENGTH-1] = 0;
}
////////////////////////////////////////////////////////////////////////////////


void storeDeviceId() {

	flashMemSegmentWrite((char*)&dsDeviceId,
		sizeof(dsDeviceId), FLASH_DEVICEID_OFFSET);
}
////////////////////////////////////////////////////////////////////////////////
