
#include "stdafx.h"

#include "crc16_ccitt.h"

//  Polynome = 0x1021; 
//  x16 + x12 + x5 + 1
//  used by SCP-ECG
unsigned short
crc_ccitt_add_byte(unsigned char bt, unsigned short crc)
{
	return CRC_CCITT(bt,crc);
}
/////////////////////////////////////////////////////////////////////////////

unsigned short
crc_ccitt_add_data(const unsigned char *data, int len, unsigned short crc)
{
	int i;
	
	for( i = 0; i < len; i++ ) {
		crc = CRC_CCITT(data[i],crc);
	}

	return crc;
}
/////////////////////////////////////////////////////////////////////////////

//  Polynome = 0x8005; 
//  x16 + x15 + x2 + 1
//  used by Contiki OS
unsigned short
crc16_add_byte(unsigned char b, unsigned short crc)
{
		crc ^= b;
		crc  = (crc >> 8) | (crc << 8);
		crc ^= (crc & 0xff00) << 4;
		crc ^= (crc >> 8) >> 4;
		crc ^= (crc & 0xff00) >> 5;
		return crc;
}
/////////////////////////////////////////////////////////////////////////////


unsigned short
crc16_add_data(const unsigned char *data, int len, unsigned short crc)
{
	int i;
	
	for( i = 0; i < len; ++i ) {
		crc = crc16_add_byte(*data++, crc);
	}

	return crc;
}
/////////////////////////////////////////////////////////////////////////////
