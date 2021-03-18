/*
 * Copyright (c) 2013-2014, Beecardia firmware.
 * All rights reserved.
 */
#include "stdafx.h" // empty file for non MSVC project

//#define TEST_PACK_METHOD

#include <string.h>
#include "compressor.h"
#include "bee_data_types.h"
#include "crc16_ccitt.h"

static short	compressor_channel_count;
static short	compressor_mode;

__int32 block_number;
__int32	total_vectors_count;
static short samples_in_block;

static unsigned short ecg_block_buffer_ddt[COMPRESS_BUF_SIZE];
static unsigned short ecg_block_buffer_raw[COMPRESS_BUF_SIZE];

typedef struct {
	unsigned short*	buffer_pointer;
	short			buffer_index;
	unsigned short	bit_buffer;
	short			bits_left_in_bitbuffer;
	bool			overflow;
	short			samples_in_block;
} compressor_st;

static compressor_st the_compressor_ddt;
static compressor_st the_compressor_raw;
#ifdef UNCOMPRESS_AVAILABLE
static compressor_st the_compressor_unpack;
#endif
static compressor_st* p_compressor;
static char* p_ecg_block;

static const unsigned short	bits_left_mask[] = {
0,
0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

static short ecg_first_prev[MAX_CHANNEL_COUNT];
static short ecg_second_prev[MAX_CHANNEL_COUNT];
////////////////////////////////////////////////////////////////////////////////

// callback functions prototype
void (* compressorSaveDataBlock)(char* buffer);

// function prototypes
static void compressorBitBufferSave(unsigned short nbits,unsigned short zdest);
static void compressorNewEcgBlock();
static void compressorFinishEcgBlock(void);
static bool compressorAddDdt(short *ecg_vector);
static bool compressorAddRaw16(short *ecg_vector);
static void compressorHuffmanSave(short val);

#ifdef UNCOMPRESS_AVAILABLE
static unsigned short compressorBitBufferFill(short bits);
static short compressorHuffmanGet(void);
static int compressorUnpackMit212(unsigned short *pblock, short *p_ecg, int samples);
static int compressorUnpackDdt(unsigned short *pblock, short *p_ecg, int samples, int adc_bits);
static int compressorUnpackRaw16(unsigned short *pblock, short *p_ecg, int samples);
#endif
////////////////////////////////////////////////////////////////////////////////

int getCompressorDataLength(void)
{
	return samples_in_block;
}
////////////////////////////////////////////////////////////////////////////////

/*
compressorInit set initial values to all critical members
*/
void compressorInit(short channel_count)
{
	compressor_channel_count = channel_count;
	compressorNewEcgBlock();
	total_vectors_count = 0;
	block_number = 0;
}
////////////////////////////////////////////////////////////////////////////////

/*
WRITE_BITS store nbits in bit buffer, if less than bits left, otherwise store bit buffer
*/
#define WRITE_BITS(nbits,zdest) \
{ \
	if (nbits <= p_compressor->bits_left_in_bitbuffer) { \
		p_compressor->bit_buffer |= ((unsigned short)zdest & bits_left_mask[nbits])<<(16-p_compressor->bits_left_in_bitbuffer); \
		p_compressor->bits_left_in_bitbuffer -= nbits; \
	} else { \
		compressorBitBufferSave(nbits,zdest); \
	} \
}
////////////////////////////////////////////////////////////////////////////////

/*
compressorBitBufferSave store the bit buffer(unsigned short value) in the compressor buffer(unsigned short[])
*/
static void compressorBitBufferSave(unsigned short nbits,unsigned short zdest)
{
	short sbits = p_compressor->bits_left_in_bitbuffer;

	if ( p_compressor->buffer_index >= COMPRESS_BUF_SIZE ) {
		p_compressor->overflow = true;
		return;
	}

	nbits -= p_compressor->bits_left_in_bitbuffer;
	p_compressor->bit_buffer |= zdest<<(16-sbits);

	p_compressor->buffer_pointer[p_compressor->buffer_index] = p_compressor->bit_buffer;
	p_compressor->buffer_index++;
	p_compressor->bit_buffer = 0;
	p_compressor->bits_left_in_bitbuffer = 16;

	p_compressor->bit_buffer |= ( (zdest>>sbits)<<(16-p_compressor->bits_left_in_bitbuffer) &
		bits_left_mask[nbits]);
	p_compressor->bits_left_in_bitbuffer -= nbits;

	if ( p_compressor->buffer_index >= COMPRESS_BUF_SIZE ) {
		p_compressor->overflow = true;
	}
}
////////////////////////////////////////////////////////////////////////////////

/*
compressorNewEcgBlock init new block variables
*/
void compressorNewEcgBlock()
{
	the_compressor_raw.bit_buffer = 0;
	the_compressor_raw.bits_left_in_bitbuffer = 16;
	the_compressor_raw.buffer_index = ( sizeof (ecg_block_header_st)  ) / 2;
	the_compressor_raw.overflow = false;
	the_compressor_raw.samples_in_block = 0;

	memcpy(&the_compressor_ddt, &the_compressor_raw, sizeof(the_compressor_ddt));

	the_compressor_raw.buffer_pointer = ecg_block_buffer_raw;
	the_compressor_ddt.buffer_pointer = ecg_block_buffer_ddt;

	samples_in_block = 0;
}
////////////////////////////////////////////////////////////////////////////////

void compressorCreateCheckSum(char* buffer)
{
	ecg_block_header_st* header;
	header = (ecg_block_header_st*)buffer;
	header->crc_ccitt = 0xffff;
	header->crc_ccitt = crc16_add_data( (unsigned char*)&header->block_size,
		header->block_size-2, header->crc_ccitt );	
}
////////////////////////////////////////////////////////////////////////////////

/*
CompressorFinishEcgBlock define the ecg data block header, and store it in the beginning of the block
*/
void compressorFinishEcgBlock(void)
{
	ecg_block_header_st* header;

#ifdef TEST_PACK_METHOD
	if ( 0 ) // 1 for raw packing, 0 - for ddt packing
#else 
	if ( the_compressor_ddt.samples_in_block < the_compressor_raw.samples_in_block )
#endif
	{
		compressor_mode = DATA_FORMAT_RAW16;
		p_ecg_block = (char*)ecg_block_buffer_raw;
	} else {
		compressor_mode = DATA_FORMAT_DDT16;
		p_ecg_block = (char*)ecg_block_buffer_ddt;
	}
	header = (ecg_block_header_st*)p_ecg_block;
	header->block_size = COMPRESS_BUF_SIZE*2;
	header->block_number = block_number;
	header->start_sample_number = total_vectors_count - samples_in_block;
	header->samples_in_block = samples_in_block;
	header->number_of_channels = (char)compressor_channel_count;
	header->data_packing_method = (char)compressor_mode;
	header->lead_off_status = 0;
}
////////////////////////////////////////////////////////////////////////////////


void compressorPushRaw32()
{
	if ( samples_in_block == 0 ) {
		return;
	}
	p_compressor = &the_compressor_raw;

	ecg_block_header_st* header = (ecg_block_header_st*)ecg_block_buffer_raw;
	header->block_size = COMPRESS_BUF_SIZE*2;
	header->block_number = block_number++;
	header->start_sample_number = total_vectors_count - samples_in_block;
	header->samples_in_block = samples_in_block;
	header->number_of_channels = (char)compressor_channel_count;
	header->data_packing_method = DATA_FORMAT_RAW32;
	header->lead_off_status = 0;
		
	compressorCreateCheckSum((char*)ecg_block_buffer_raw);
		
	if ( compressorSaveDataBlock ) {
		compressorSaveDataBlock((char*)ecg_block_buffer_raw);			
	}
		
	compressorNewEcgBlock();
}
////////////////////////////////////////////////////////////////////////////////


void compressorAddRaw32(__int32 data)
{
	p_compressor = &the_compressor_raw;

	__int32* pbuff = (__int32*)&p_compressor->buffer_pointer[p_compressor->buffer_index];
	*pbuff = data;
	total_vectors_count++;
	samples_in_block++;
	p_compressor->buffer_index +=2;

	if ( p_compressor->buffer_index >= COMPRESS_BUF_SIZE-1 ) {

		ecg_block_header_st* header = (ecg_block_header_st*)ecg_block_buffer_raw;
		header->block_size = COMPRESS_BUF_SIZE*2;
		header->block_number = block_number++;
		header->start_sample_number = total_vectors_count - samples_in_block;
		header->samples_in_block = samples_in_block;
		header->number_of_channels = (char)compressor_channel_count;
		header->data_packing_method = DATA_FORMAT_RAW32;
		header->lead_off_status = 0;
		
		compressorCreateCheckSum((char*)ecg_block_buffer_raw);

		if ( compressorSaveDataBlock ) {
			compressorSaveDataBlock((char*)ecg_block_buffer_raw);			
		}

		compressorNewEcgBlock();
	}
}
////////////////////////////////////////////////////////////////////////////////


/*
compressorAddVector stores ecg in two ways: compressed and uncompressed.
If both buffers will be full, buffer which have more data will be push forward.
*/
void compressorAddVector(short* ecg_vector)
{
	compressorAddRaw16(ecg_vector);
	compressorAddDdt(ecg_vector);

#ifdef TEST_PACK_METHOD
	if ( the_compressor_raw.overflow  ) // change _raw to _ddt if needed
#else 
	if ( the_compressor_ddt.overflow && the_compressor_raw.overflow  )
#endif
	{
		compressorFinishEcgBlock();
		// send block to parent
		if ( compressorSaveDataBlock ) {
			compressorSaveDataBlock( p_ecg_block );			
		}
		compressorNewEcgBlock();
		block_number++;

		compressorAddDdt(ecg_vector);
		compressorAddRaw16(ecg_vector);
	}

	total_vectors_count++;
	samples_in_block++;
}
////////////////////////////////////////////////////////////////////////////////

/*
Compressor Close() Push incomplete block
returns count of successfully stored samples
*/
__int32 compressorClose(void)
{
	if ( samples_in_block != 0 ) {
		compressorFinishEcgBlock();
		// send block to parent
		if ( compressorSaveDataBlock ) {
			compressorSaveDataBlock( p_ecg_block );			
		}
	}

	return total_vectors_count;
}
////////////////////////////////////////////////////////////////////////////////


static bool compressorAddRaw16(short *ecg_vector)
{
	short j;
	if ( the_compressor_raw.overflow ) {
		return false;
	}
	p_compressor = &the_compressor_raw;
	
	for ( j = 0; j < compressor_channel_count; j++ ) {
		p_compressor->buffer_pointer[p_compressor->buffer_index++] = ecg_vector[j];

		if ( p_compressor->buffer_index >= COMPRESS_BUF_SIZE ) {
			p_compressor->overflow = true;
			break;

		}
	}
	
	if ( p_compressor->overflow == false ) {
		p_compressor->samples_in_block++;
	}

	return (p_compressor->overflow == false);
}
////////////////////////////////////////////////////////////////////////////////


/*
compressor_AddVector add the ECG vector to buffer
first value is data itself
second - first derivative
next  - the second derivative
return value - true if success
*/
static bool compressorAddDdt(short *ecg_vector)
{
	short j;
	if ( the_compressor_ddt.overflow ) {
		return false;
	}
	p_compressor = &the_compressor_ddt;

	for ( j = 0; j < compressor_channel_count; j++ ) {
		switch( p_compressor->samples_in_block )
		{
		case 0: WRITE_BITS( 16, (ecg_vector[j]) );
			break;
		case 1: compressorHuffmanSave( ecg_vector[j] - ecg_first_prev[j] );
			break;
		default: compressorHuffmanSave( ecg_vector[j] - (ecg_first_prev[j]<<1) + ecg_second_prev[j] );
			break;
		}
	}
	memcpy( ecg_second_prev, ecg_first_prev, sizeof(ecg_second_prev) );
	memcpy( ecg_first_prev,  ecg_vector,     sizeof(ecg_first_prev)  );

	if ( p_compressor->overflow == false ) {
		p_compressor->samples_in_block++;
	}
	return (p_compressor->overflow == false);
}
////////////////////////////////////////////////////////////////////////////////

/*
compressor_Huffman_save
implement Huffman code table
*/
static void compressorHuffmanSave(short val)
{
	short temp = val;
	if ( temp < 0 ) temp--;

	if ( temp == 0 )						{ WRITE_BITS(1, 0 );  return; }
	if ( temp <= 1 && temp >= -2 )			{ WRITE_BITS(2,	1 );	WRITE_BITS(1, temp );	return;} // 3
	if ( temp <= 3 && temp >= -4 )			{ WRITE_BITS(3,	3 );	WRITE_BITS(2, temp );	return;} // 5
	if ( temp <= 7 && temp >= -8 )			{ WRITE_BITS(4,	7 );	WRITE_BITS(3, temp );	return;} // 7
	if ( temp <= 15 && temp >= -16 )		{ WRITE_BITS(5,	15 );	WRITE_BITS(4, temp );	return;} // 9
	if ( temp <= 31 && temp >= -32 )		{ WRITE_BITS(6,	31 );	WRITE_BITS(5, temp );	return;} // 11
	if ( temp <= 63 && temp >= -64 )		{ WRITE_BITS(7,	63 );	WRITE_BITS(6, temp );	return;} // 13
	if ( temp <= 127 && temp >= -128 )		{ WRITE_BITS(8,	127 );	WRITE_BITS(7, temp );	return;} // 15
											{ WRITE_BITS(9,	255 );	WRITE_BITS(16,temp );}
}
////////////////////////////////////////////////////////////////////////////////


#ifdef UNCOMPRESS_AVAILABLE
#define READ_BITS(nbits,zdest) \
{ \
	if ( nbits <= p_compressor->bits_left_in_bitbuffer) { \
		zdest = ( unsigned short)(p_compressor->bit_buffer & bits_left_mask[nbits]); \
		p_compressor->bit_buffer >>= nbits; p_compressor->bits_left_in_bitbuffer -= nbits; \
	} else { \
		zdest = compressorBitBufferFill(nbits); \
	} \
}
////////////////////////////////////////////////////////////////////////////////


static unsigned short compressorBitBufferFill(short bits)
{
	//  get the bits that are left and read the next word 
	register unsigned short result = p_compressor->bit_buffer;
	register short sbits = p_compressor->bits_left_in_bitbuffer;
	bits -= p_compressor->bits_left_in_bitbuffer;
	
	if ( p_compressor->buffer_index < COMPRESS_BUF_SIZE ) {
		p_compressor->bit_buffer = p_compressor->buffer_pointer[p_compressor->buffer_index++];
		p_compressor->bits_left_in_bitbuffer = 16;
	} else {
		p_compressor->overflow = true;
		return 0;
	}

	// get the remaining bits 
	result = result | (unsigned short) ((p_compressor->bit_buffer & bits_left_mask[bits]) << sbits);
	p_compressor->bit_buffer >>= bits;
	p_compressor->bits_left_in_bitbuffer -= bits;
	return result;
}
////////////////////////////////////////////////////////////////////////////////

/*
CompressorHuffmanGet return short value from Huffman bit stream
*/
static short compressorHuffmanGet(void)
{
	short cc;
	READ_BITS(1,cc); if (cc==0) return 0; //0
	READ_BITS(1,cc); if (cc==0) { READ_BITS(1,cc)  if ((cc & 0x0001)==0) cc=(cc|0xfffe)+1; return cc; }//1    // 10
	READ_BITS(1,cc); if (cc==0) { READ_BITS(2,cc)  if ((cc & 0x0002)==0) cc=(cc|0xfffc)+1; return cc; }//3    // 110
	READ_BITS(1,cc); if (cc==0) { READ_BITS(3,cc)  if ((cc & 0x0004)==0) cc=(cc|0xfff8)+1; return cc; }//7    // 1110
	READ_BITS(1,cc); if (cc==0) { READ_BITS(4,cc)  if ((cc & 0x0008)==0) cc=(cc|0xfff0)+1; return cc; }//15   // 11110
	READ_BITS(1,cc); if (cc==0) { READ_BITS(5,cc)  if ((cc & 0x0010)==0) cc=(cc|0xffe0)+1; return cc; }//31   // 111110
	READ_BITS(1,cc); if (cc==0) { READ_BITS(6,cc)  if ((cc & 0x0020)==0) cc=(cc|0xffc0)+1; return cc; }//63   // 1111110
	READ_BITS(1,cc); if (cc==0) { READ_BITS(7,cc)  if ((cc & 0x0040)==0) cc=(cc|0xff80)+1; return cc; }//127  // 11111110
	
	READ_BITS(1,cc); if (cc==0) { READ_BITS(16,cc); if (cc<0) cc++; return cc;}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////

static int compressorUnpackMit212(unsigned short *pblock, short *p_ecg, int samples)
{
	short temp_value;
	ecg_block_header_st* p_header = (ecg_block_header_st*)pblock;
	
	int samples_to_read = p_header->samples_in_block;
	if (samples < samples_to_read ) samples_to_read = samples;

	int j, index = 0;
	while ( index < samples_to_read) {
		for ( j = 0; j < p_header->number_of_channels; j++ ) {
			READ_BITS(12, temp_value);
			if (temp_value & 0x800) temp_value |= 0xf800;
			*p_ecg++ = temp_value;
		}
		
		if ( p_compressor->overflow ) {
			break;
		}
		index++;
	}
	return index;
}
////////////////////////////////////////////////////////////////////////////////


static int compressorUnpackRaw16(unsigned short *pblock, short *p_ecg, int samples)
{
	ecg_block_header_st* p_header = (ecg_block_header_st*)pblock;
	
	int samples_to_read = p_header->samples_in_block;
	if (samples < samples_to_read ) samples_to_read = samples;
	
	int j, index = 0;
	while ( index < samples_to_read) {
		for ( j = 0; j < p_header->number_of_channels; j++ ) {
			if ( p_compressor->buffer_index < COMPRESS_BUF_SIZE ) {
				*p_ecg++ = p_compressor->buffer_pointer[p_compressor->buffer_index++];
			} else {
				p_compressor->overflow = true;
			}
		}
		if ( p_compressor->overflow ) {
			break;
		}
		index++;
	}
	return index;
}
////////////////////////////////////////////////////////////////////////////////


static int compressorUnpackDdt(unsigned short *pblock, short *p_ecg, int samples, int adc_bits)
{
	short temp_value;
	ecg_block_header_st* p_header = (ecg_block_header_st*)pblock;

	short x_m0[MAX_CHANNEL_COUNT];
	short x_m1[MAX_CHANNEL_COUNT];
	short x_m2[MAX_CHANNEL_COUNT];
	
	int samples_to_read = p_header->samples_in_block;
	if (samples < samples_to_read ) samples_to_read = samples;
	
	int j, index = 0;
	while ( index < samples_to_read ) {
		switch (index) {
		case 0:
			for ( j = 0; j < p_header->number_of_channels; j++ ) {
				READ_BITS(adc_bits, temp_value);

				if (adc_bits == 12 && temp_value & 0x800) temp_value |= 0xf800;
				x_m0[j] = temp_value;
			}
			break;
		case 1:
			for ( j = 0; j < p_header->number_of_channels; j++ ) {
				x_m0[j] = CompressorHuffmanGet() + x_m1[j];
			}
			break;
		default:
			for ( j = 0; j < p_header->number_of_channels; j++ ) {
				x_m0[j] = CompressorHuffmanGet() + (x_m1[j]<<1) - x_m2[j];
			}
			break;
		}
		
		memcpy(x_m2, x_m1, sizeof(x_m2));
		memcpy(x_m1, x_m0, sizeof(x_m1));
		
		for ( j = 0; j < p_header->number_of_channels; j++ ) {
			*p_ecg++ = x_m0[j];
		}

		if ( p_compressor->overflow ) {
			break;
		}
		index++;
	}

	return index;
}
////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
bool test_blkock = false;
int  block_nom = 0;
#endif  // _DEBUG

int compressorUnpackBlock(unsigned short *pblock, short *p_ecg, int samples)
{
	ecg_block_header_st* p_header = (ecg_block_header_st*)pblock;
	int samples_read = 0;

#ifdef _DEBUG
	if ( test_blkock ) {
		if ( block_nom+1 != p_header->block_number ) {
//			TRACE(_T("__ERROR block nomber %d, start sample = %d\n"),
//			block_nom, p_header->start_sample_number);
		}
		block_nom = p_header->block_number;
	}
#endif  // _DEBUG

	memset(&the_compressor_unpack, 0, sizeof(the_compressor_unpack));
	p_compressor = &the_compressor_unpack;
	p_compressor->buffer_index = ( sizeof (ecg_block_header_st)  ) / sizeof(short);
	p_compressor->buffer_pointer = pblock;

	switch ( p_header->data_packing_method ) {
	case DATA_FORMAT_DDT12:
		samples_read = CompressorUnpackDdt((unsigned short*)pblock, p_ecg, samples, 12);
		break;
	case DATA_FORMAT_DDT16:
		samples_read = CompressorUnpackDdt((unsigned short*)pblock, p_ecg, samples, 16);
		break;
	case DATA_FORMAT_MIT212: 
		samples_read = CompressorUnpackMit212((unsigned short*)pblock, p_ecg, samples);
		break;
	case DATA_FORMAT_RAW16: 
		samples_read = CompressorUnpackRaw16((unsigned short*)pblock, p_ecg, samples);
		break;
	}

	return samples_read;
}
////////////////////////////////////////////////////////////////////////////////
#endif //UNCOMPRESS_AVAILABLE
