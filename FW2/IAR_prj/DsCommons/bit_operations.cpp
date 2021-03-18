/*
Copyright (C) 2013
*/

#include "stdafx.h"

#include "bit_operations.h"


int MostSignificantBitOnly(int x)
{
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
   return x - (x >> 1);
}
/////////////////////////////////////////////////////////////////////////////

/*
MakeNegative makes most significant bit extension
parameters:
value input - value
adc_bits - most significant bit number
return value - modified input
*/
int MakeNegative(int value, int adc_bits)
{
	int ret = value;
	if ( checkbit(ret,(adc_bits-1)) ) ret |= (0xffffffff << ( adc_bits ));
	return ret;
}
/////////////////////////////////////////////////////////////////////////////
