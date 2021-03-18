/*
Copyright (C) 2013
*/

#include "stdafx.h"
#include "types_porting.h"

/*
short_to_int copy array of shorts to array of int values
*/
void short_to_int(int* i_valueas, short* s_values, int count)
{
	for ( int i = 0; i < count; i++) {
		i_valueas[i] = s_values[i];
	}
}
/////////////////////////////////////////////////////////////////////////////


unsigned short bytes2short(char lsb, char msb)
{
	union {
		char bytes[2] ;
		unsigned short s_valur;
	} b2s;
		
	b2s.bytes[0] = lsb;
	b2s.bytes[1] = msb;
	
	return b2s.s_valur;
}
///////////////////////////////////////////////////////////////////////////////


void short2bytes(char* p_array, short value)
{

	p_array[0] = *( (char*)&value );
	p_array[1] = *( (char*)&value+1 );

}
///////////////////////////////////////////////////////////////////////////////
