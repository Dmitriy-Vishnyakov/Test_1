/*
Copyright (C) 2013
*/

#ifndef _BIT_OPERATIONS_H_
#define _BIT_OPERATIONS_H_

#define	checkbit(var,bit)	(var & (0x01<<(bit)))
#define	setbit(var,bit)		(var |=(0x01<<(bit)))
#define	clrbit(var,bit)		(var &=(~(0x01<<(bit))))
#define	invertbit(var,bit)	(var ^=(0x01<<(bit))) 

#define	BYTE0(var)	   *(((char*)&(var))+0)
#define	BYTE1(var)	   *(((char*)&(var))+1)
#define	BYTE2(var)	   *(((char*)&(var))+2)
#define	BYTE3(var)	   *(((char*)&(var))+3)

#define	SHORT0(var)	   *(((short*)&(var))+0)
#define	SHORT1(var)	   *(((short*)&(var))+1)

int MostSignificantBitOnly(int x);
int MakeNegative(int value, int adc_bits);

#endif // _BIT_OPERATIONS_H_
