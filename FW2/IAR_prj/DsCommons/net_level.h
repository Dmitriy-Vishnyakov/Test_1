/*
 * Copyright (c) 2013,
 * All rights reserved.
 */

#include "types_porting.h"

#ifndef _NET_LEVEL_H_
#define _NET_LEVEL_H_

#define IO_DELIM_START                  0x02
#define IO_DELIM_END                    0x03
#define IO_PREFIX_LENGTH                0x06
#define IO_COMMAND_CODE_POS             0x02

#define BYTE_OPCODE                     2

#define INPUT_STATUS_WAIT_START         0
#define INPUT_STATUS_WAIT_COMMAND       1
#define INPUT_STATUS_WAIT_HEADER        2
#define INPUT_STATUS_DATA_STREAM        3
#define INPUT_STATUS_WAIT_END           4

#define IO_PACKET_TYPE_RESPONSE         0x42
#define IO_PACKET_TYPE_REQUEST          0x52
#define IO_PACKET_TYPE_CONFIRM          0x43
#define IO_PACKET_TYPE_INDICATION       0x69
#define IO_PACKET_TYPE_RESPONSE_OPT     0x72

#define IO_DEFAULT_RESPONSE				      0xfb
#define IO_DEFAULT_REQUEST				      0xfc

#define IO_OPCODE_SPP_LINK_ESTABLISHED  0x0b
#define IO_OPCODE_SPP_INCOMMING_LINK_ESTABLISHED 0x0c
#define IO_OPCODE_SPP_RELEASE_LINK      0x0d
#define IO_OPCODE_SPP_LINK_RELEASED     0x0e

#ifdef __IAR_SYSTEMS_ICC__
#define INPUT_BUFFER_LENGTH             64*4
#endif // __IAR_SYSTEMS_ICC__

#if _MSC_VER > 1000
// we should be able to receive blocks with 512 length on Win machine
#define INPUT_BUFFER_LENGTH             1024
#endif // _MSC_VER > 1000

#define MAX_PREFEX_LENGTH               8

#define ERROR_BUFFER_LENGTH             16
#define ERROR_START_PACKET              (char)0xaa

#if _MSC_VER > 1000
#define LOG_BT_INPUT(error)
#else
#define LOG_BT_INPUT(error) { bt_err_index = (bt_err_index+1)&(ERROR_BUFFER_LENGTH-1);  bt_err_buffer[bt_err_index] = (error); }
#endif

#define MSG_PAC_TYPE     received_msg_buffer[1]
#define MSG_OPPCODE      received_msg_buffer[2]
#define MSG_DATA_LENGTH  *((short*)&received_msg_buffer[3])


typedef struct {
	unsigned char Buf[INPUT_BUFFER_LENGTH];
	unsigned char bb[MAX_PREFEX_LENGTH];
	short indexBuf;
	unsigned char status;
	unsigned char crc;
	short index;
	short len_leave;
} input_chain_st;

void  NetLevelInit(void);
short NetLevelWrapData(char* output_buffer, char* input_buffer, short data_length);
short NetLevelReadData(char* buffer);
bool  NetLevelPushByteToReceiver(unsigned char in_byte);
short NetLevelAddIncomingByte(unsigned char in_byte);
bool NetLevelIsCrcCorrect(char* received_msg);
void NetLevelCreateResponse(char* response_buffer, char* data, short data_length, unsigned short opp_code);
void NetLevelGetMessageData(char* message, char* data, short data_length);

extern char bt_err_buffer[ERROR_BUFFER_LENGTH];
extern unsigned char bt_err_index;

#endif // _NET_LEVEL_H_