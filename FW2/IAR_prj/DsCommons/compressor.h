/*
Beecardia firmware.
*/
#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_

#include "types_porting.h"

#define COMPRESS_BUF_SIZE   256

#define ECG_RESULTING_BITS  12

////////////////////////////////////////////////////////////////////////////////
// Compressor interface
void compressorInit(short channel_count);
void compressorAddVector(short* ecg_vector);
void compressorAddRaw32(__int32 data);
void compressorPushRaw32();
void compressorCreateCheckSum(char* buffer);
__int32 compressorClose(void);
int getCompressorDataLength(void);

#ifdef UNCOMPRESS_AVAILABLE
int compressorUnpackBlock(unsigned short *pblock, short *p_ecg, int samples);
#endif

// call back prototype
extern void (* compressorSaveDataBlock)(char* buffer);

#endif // _COMPRESSOR_H_
