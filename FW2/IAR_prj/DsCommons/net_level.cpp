/*
 * Copyright (c) 2013,
 * All rights reserved.
 */
#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include "net_level.h"
#include "bee_data_types.h"
#include "crc16_ccitt.h"

#if _MSC_VER > 1000

// win version need longer buffer
#include "../AppLog/app_log.h"
char BTBuffer0[512*2];
char BTBuffer1[512*2];

#else // else FW version

#ifdef __IAR_SYSTEMS_ICC__
#pragma data_alignment=2
#endif
char BTBuffer0[64*2];

#ifdef __IAR_SYSTEMS_ICC__
#pragma data_alignment=2
#endif
char BTBuffer1[64*2];

#endif // win or FW version

char* received_msg_buffer;
char* p_current_bt_buffer = BTBuffer0;

input_chain_st input_chain;

static short NetLevelAddPrefix(char* buffer, short data_length);
static short NetLevelAddSufix(char* buffer);
static unsigned char packet_type;

char bt_err_buffer[ERROR_BUFFER_LENGTH];
unsigned char bt_err_index;

bool incoming_link;

///////////////////////////////////////////////////////////////////////////////


void NetLevelInit(void)
{
	memset((char*)&input_chain, 0, sizeof input_chain);
}
///////////////////////////////////////////////////////////////////////////////


void NetLevelGetMessageData(char* message, char* data, short data_length)
{
	message_header_st* p_request_msg = (message_header_st*)message;
	
	short length = p_request_msg->data_length;
	if ( length > data_length ) {
		length = data_length;
	}
	memcpy(data, &received_msg_buffer[sizeof(message_header_st)], length);
}
///////////////////////////////////////////////////////////////////////////////


void NetLevelCreateResponse(char* response_buffer, char* data, short data_length, unsigned short opp_code)
{
	message_header_st* p_msg = (message_header_st*)response_buffer;

	// copy	data
	memcpy(&response_buffer[sizeof(message_header_st)], data, data_length);
	p_msg->opcode = opp_code;
	short2bytes((char*)(&p_msg->data_length), data_length);
//	p_msg->data_length = data_length;
	
	// calculate check sum
	unsigned short crc = 0xffff;
	short ckeck_length = p_msg->data_length + 2 + sizeof(short);
	crc = crc16_add_data( (unsigned char*)&p_msg->opcode, ckeck_length, crc );
	short2bytes((char*)(&p_msg->crc), crc);
//	p_msg->crc = crc;

	// wrap response by start and end delimiters
	p_msg->net_prefix[0] = IO_DELIM_START;
	p_msg->net_prefix[1] = IO_PACKET_TYPE_RESPONSE;
	p_msg->net_prefix[2] = IO_DEFAULT_RESPONSE;
	
	// message length without start and stop delimiters
	short msg_length = p_msg->data_length + sizeof(p_msg->net_prefix);
	p_msg->net_prefix[3] = msg_length & 0xff;
	p_msg->net_prefix[4] = msg_length >> 8;
	p_msg->net_prefix[5] = p_msg->net_prefix[1] + p_msg->net_prefix[2] + p_msg->net_prefix[3] + p_msg->net_prefix[4];
	response_buffer[p_msg->data_length + sizeof(message_header_st)] = IO_DELIM_END;
}
///////////////////////////////////////////////////////////////////////////////


bool NetLevelIsCrcCorrect(char* received_msg)
{
	message_header_st* p_msg = (message_header_st*)received_msg;
	
	unsigned short crc = 0xffff;
	short ckeck_length = p_msg->data_length + 2 + sizeof(short);
	crc = crc16_add_data( (unsigned char*)&p_msg->opcode, ckeck_length, crc );

	return (p_msg->crc == crc);
}
///////////////////////////////////////////////////////////////////////////////


short NetLevelAddPrefix(char* buffer, short data_length)
{
	unsigned char lmx_header[] = {IO_DELIM_START, IO_PACKET_TYPE_REQUEST,  IO_DEFAULT_REQUEST, 0, 0, 0};

	lmx_header[3] = data_length & 0xff;
	lmx_header[4] = data_length >> 8;
	lmx_header[5] = lmx_header[1] + lmx_header[2] + lmx_header[3] + lmx_header[4];

	memcpy(buffer, lmx_header, IO_PREFIX_LENGTH);

	return IO_PREFIX_LENGTH;
}
///////////////////////////////////////////////////////////////////////////////


short NetLevelAddSufix(char* buffer)
{
	*buffer = IO_DELIM_END;

	return 1;
}
///////////////////////////////////////////////////////////////////////////////


short NetLevelWrapData(char* output_buffer, char* input_buffer, short data_length)
{
	short bytes_to_send;

	bytes_to_send = NetLevelAddPrefix(output_buffer, data_length);
	memcpy(&output_buffer[bytes_to_send], input_buffer, data_length);
	bytes_to_send += data_length;
	bytes_to_send += NetLevelAddSufix(&output_buffer[bytes_to_send]);

	return bytes_to_send;
}
///////////////////////////////////////////////////////////////////////////////


short NetLevelReadData(char* buffer)
{
	memcpy(buffer, input_chain.Buf, input_chain.indexBuf);
	return input_chain.indexBuf;
}
///////////////////////////////////////////////////////////////////////////////

/*
lmx9838 data packet parser, we need something for byte stream synchronization
(start delimiter, packet type, command, length... end delimiter )
*/
bool NetLevelPushByteToReceiver(unsigned char in_byte)
{
	bool ret = false;

//	PRINT_LOG( "0x%.2x ", in_byte );

	switch ( input_chain.status )
	{
	case INPUT_STATUS_WAIT_START: //  wait IO_DELIM_START
		if (in_byte == IO_DELIM_START) {
			input_chain.crc = 0;
			input_chain.index = 0;
			input_chain.bb[input_chain.index++] = IO_DELIM_START;
			input_chain.len_leave = IO_PREFIX_LENGTH-1;
			input_chain.status = INPUT_STATUS_WAIT_COMMAND;
//			PRINT_LOG( "\n" );
		}
		break;

	case INPUT_STATUS_WAIT_COMMAND: // got command
		if (input_chain.index == 1) {
			if ( (in_byte != IO_PACKET_TYPE_REQUEST) &&
				 (in_byte != IO_PACKET_TYPE_CONFIRM) &&
				 (in_byte != IO_PACKET_TYPE_RESPONSE) &&
				 (in_byte != IO_PACKET_TYPE_INDICATION) ) {
				input_chain.status = INPUT_STATUS_WAIT_START;
				LOG_BT_INPUT(1);
				break;
			}
			packet_type = in_byte;
		}

		if (input_chain.len_leave) {
			input_chain.bb[(input_chain.index++) & 7] = in_byte;
			if ( input_chain.index < IO_PREFIX_LENGTH ) {
				input_chain.crc += in_byte;
			}
			input_chain.len_leave--;
		}

		if (input_chain.len_leave == 0) {
			input_chain.len_leave = input_chain.bb[3] + 256*input_chain.bb[4];
			if ( input_chain.len_leave ) {
				input_chain.status = INPUT_STATUS_DATA_STREAM;
			} else  {
				input_chain.status = INPUT_STATUS_WAIT_END;
			}
			
			input_chain.indexBuf = 0;
			if ( (in_byte != input_chain.crc) ) {
				input_chain.status = INPUT_STATUS_WAIT_START;
				LOG_BT_INPUT(2);
				ret = false;
			}
		}
		break;

	case INPUT_STATUS_DATA_STREAM: //  read bytes
		if ( input_chain.indexBuf < INPUT_BUFFER_LENGTH ) {
			input_chain.Buf[input_chain.indexBuf++] = in_byte;
		}
		if ( --input_chain.len_leave == 0 ) {
			input_chain.status = INPUT_STATUS_WAIT_END;
		}
		break;

	case INPUT_STATUS_WAIT_END: // Wait IO_DELIM_END
		if ( in_byte == IO_DELIM_END  ) {
#ifdef _MSC_VER
			if  ( packet_type == IO_PACKET_TYPE_RESPONSE ) {
				ret = true;
			}
#endif
			if  ( packet_type == IO_PACKET_TYPE_REQUEST ) {
				ret = true;
			}
			
			if  ( ( packet_type == IO_PACKET_TYPE_INDICATION ) || ( packet_type == IO_PACKET_TYPE_CONFIRM )
					 || ( packet_type == IO_PACKET_TYPE_RESPONSE ) ) {
				LOG_BT_INPUT(packet_type);
				LOG_BT_INPUT(input_chain.bb[BYTE_OPCODE]);
				if ( input_chain.bb[BYTE_OPCODE] == IO_OPCODE_SPP_LINK_RELEASED ) {
					incoming_link = false;
				}
				
				if ( input_chain.bb[BYTE_OPCODE] == IO_OPCODE_SPP_INCOMMING_LINK_ESTABLISHED ) {
					incoming_link = true;
				}				
			}
		} else {
 			LOG_BT_INPUT(3);
		}
//		PRINT_LOG( "\n" );
		input_chain.status = INPUT_STATUS_WAIT_START;
		input_chain.len_leave = 0;
		break;

	default: // never reach it
		LOG_BT_INPUT(4);
//		PRINT_LOG( "\n" );
		input_chain.status = INPUT_STATUS_WAIT_START;
		input_chain.len_leave = 0;
		break;
	}
	return ret;
}
/////////////////////////////////////////////////////////////////////////////


/*
lmx9838 data packet parser, we need something for byte stream synchronization
(start delimiter, packet type, command, length... end delimiter )
*/

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

short NetLevelAddIncomingByte(unsigned char in_byte)
{
	static char status_bt = INPUT_STATUS_WAIT_START;
	static short index_bt = 0;
	static unsigned char crc_bt = 0;
	static short message_length_bt = 0;
	
	short ret = -1;
	
        
	switch ( status_bt ) {
	case INPUT_STATUS_WAIT_START: //  wait IO_DELIM_START
		if ( in_byte == IO_DELIM_START ) {
			index_bt = 0;
			crc_bt = 0;
			p_current_bt_buffer[index_bt++] = in_byte;
			status_bt = INPUT_STATUS_WAIT_COMMAND;
//			TRACE("_Got start delimiter\n");
		}
		break;
		
	case INPUT_STATUS_WAIT_COMMAND: // got command
		if ( (in_byte != IO_PACKET_TYPE_REQUEST) &&
			(in_byte != IO_PACKET_TYPE_CONFIRM) &&
			(in_byte != IO_PACKET_TYPE_RESPONSE) &&
			(in_byte != IO_PACKET_TYPE_INDICATION) ) {
//			TRACE("_NOT a command after start delimiter\n");
			status_bt = INPUT_STATUS_WAIT_START;
			break;
		}
//		TRACE("_Got IO_PACKET_TYPE  %X\n", in_byte);
		p_current_bt_buffer[index_bt++] = in_byte;
		status_bt = INPUT_STATUS_WAIT_HEADER;
		crc_bt += in_byte;
		
		break;
		
	case INPUT_STATUS_WAIT_HEADER:
		p_current_bt_buffer[index_bt++] = in_byte;
		
		if ( index_bt == IO_PREFIX_LENGTH ) { // CRC position
			if ( crc_bt != in_byte ) {
				status_bt = INPUT_STATUS_WAIT_START;
//				TRACE("_Got bad crc\n");
				break;
			}
			
			message_length_bt = bytes2short(p_current_bt_buffer[3], p_current_bt_buffer[4]) + IO_PREFIX_LENGTH + 1;
//			message_length_bt = *((short*)&p_current_bt_buffer[3]) + IO_PREFIX_LENGTH + 1;
			if ( message_length_bt > INPUT_BUFFER_LENGTH ) {
//				TRACE("_TOO long datalength request\n");
				status_bt = INPUT_STATUS_WAIT_END;
			} else {
//				TRACE("_Got message_length_bt  %d\n", message_length_bt);
				status_bt = INPUT_STATUS_DATA_STREAM;
			}
		} else {
			crc_bt += in_byte;
		}
		break;
		
	case INPUT_STATUS_DATA_STREAM: //  read bytes
		p_current_bt_buffer[index_bt++] = in_byte;
		
		if ( index_bt >= message_length_bt-1 ) {
//			TRACE("_end stream data\n");
			status_bt = INPUT_STATUS_WAIT_END;
		}
		break;
		
	case INPUT_STATUS_WAIT_END:
		if ( in_byte == IO_DELIM_END ) {
			p_current_bt_buffer[index_bt++] = in_byte;
			ret = message_length_bt;
			received_msg_buffer = p_current_bt_buffer;
			if ( p_current_bt_buffer == BTBuffer0 ) {
				p_current_bt_buffer = BTBuffer1;
			} else {
				p_current_bt_buffer = BTBuffer0;
			}
//			TRACE("_Got end delimiter\n");
		} else {
//			TRACE("_NO end delimiter\n");
		}
		status_bt = INPUT_STATUS_WAIT_START;
		break;
		
	default:
		input_chain.status = INPUT_STATUS_WAIT_START;
		break;
	}
	
	return ret;
}
/////////////////////////////////////////////////////////////////////////////
