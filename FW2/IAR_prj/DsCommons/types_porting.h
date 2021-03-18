// int32 and bool support

#ifndef _TYPES_PORTING_H_
#define _TYPES_PORTING_H_

#ifdef __GNUC__
#include <cstdlib>
#include "stdint.h"
#define __int32 int32_t
#define __int64 int64_t
#endif // __GNUC__

#ifdef __IAR_SYSTEMS_ICC__
#define __int32 long
#define __int64 long long

#ifndef    bool
#define    bool    short
#endif

#ifndef    false
#define    false   0
#endif

#ifndef    true
#define    true    1
#endif

#endif  // __IAR_SYSTEMS_ICC__ // bool support in IAR

typedef struct {
	int  x;
	int  y;
} point_st;
/////////////////////////////////////////////////////////////////////////////

// declare mul_div.
inline int mul_div(int number, int numerator, int denominator);
// then it's body
inline int mul_div(int number, int numerator, int denominator) {
    __int64 ret = number;
    ret *= numerator;
    ret /= denominator;
    return (int) ret;
}
/////////////////////////////////////////////////////////////////////////////

#ifndef _MSC_VER
 #ifndef   INT_MAX
 #define   INT_MAX 2147483647
 #endif // INT_MAX

 #ifndef   INT_MIN
 #define   INT_MIN (-2147483647-1)
 #endif
#else // win version
 #include <limits.h>
#endif  // _MSC_VER

#ifndef   SHORT_MAX
 #define   SHORT_MAX 32767
#endif // SHORT_MAX

#ifndef   SHORT_MIN
 #define   SHORT_MIN -32768
#endif // SHORT_MIN

#ifndef   BYTE_MAX
 #define   BYTE_MAX  255
#endif // BYTE_MAX

#ifndef MAX_ECG_LEADS_COUNT // should be define in the top level module (fix me later)
 #define MAX_ECG_LEADS_COUNT   16
#endif

#ifndef MAX_CHANNEL_COUNT
 #define MAX_CHANNEL_COUNT     8
#endif

#define BIT0                (0x0001u)
#define BIT1                (0x0002u)
#define BIT2                (0x0004u)
#define BIT3                (0x0008u)
#define BIT4                (0x0010u)
#define BIT5                (0x0020u)
#define BIT6                (0x0040u)
#define BIT7                (0x0080u)
#define BIT8                (0x0100u)
#define BIT9                (0x0200u)
#define BITA                (0x0400u)
#define BITB                (0x0800u)
#define BITC                (0x1000u)
#define BITD                (0x2000u)
#define BITE                (0x4000u)
#define BITF                (0x8000u)


void short_to_int(int* i_valueas, short* s_values, int count);
unsigned short bytes2short(char lsb, char msb);
void short2bytes(char* p_array, short value);

#endif // _TYPES_PORTING_H_
