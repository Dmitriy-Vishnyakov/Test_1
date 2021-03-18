/*
 * Copyright (c) 2013-2014 Beecardia firmware.
 * All rights reserved.
 */

#ifndef _BEE_DATA_TYPES_H_
#define	_BEE_DATA_TYPES_H_

#define VERSION_NAME_LENGTH         32
#define DEVICE_ID_NAME_LENGTH       24
#define LEAD_NAME_LENGTH            16
#define MAX_LEAD_COUNT              16
#define SHA1_ID_LENGTH              44
#define LEADS_BUFFER_LENGTH         64
#define DEFAULT_ECGDATA_BLOCK_SIZE  512

#define MAX_LEADS_IN_VIEW           12

typedef struct
{
	unsigned char adr[6];
	char name[64];
} BlueDeviceId;
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
typedef struct {
	char device_class[DEVICE_ID_NAME_LENGTH];
	char device_number[DEVICE_ID_NAME_LENGTH];
} DsDeviceId;
/////////////////////////////////////////////////////////////////////////////

#include "types_porting.h"

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)     /* set alignment to 2 byte boundary */
typedef struct {
	__int64             uts_gmt_timestamp; // Unix time stamp UTS (GMT) seconds since (1970,1,1,0,0,0)
	__int32             local_time_bias;   // time zone bias in minutes range from -12*60 to 12*60
} bee_data_time_st;
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	char                format_version[32];     // "Beecardia ECG v1"

	__int32             time_index_table_offset;// we need it for fast find start sample
	__int32             time_index_table_size;  // 8192 (2048 int values)

	__int32             ecg_data_offset;
	__int32             ecg_data_size;          // bytes

	__int32             primary_events_offset;  // lead off/on, patient button press e.t.c. info
	__int32             primary_events_size;    // bytes

	__int32             user_def_data_offset;   // future extension
	__int32             user_def_data_size;     // bytes

	bee_data_time_st    record_start_time;      // 12 bytes UTS (GMT) and time zone bias
	
	__int32             record_duration;        // [ms]
	__int32             sampling_rate;          // [Hz].
	__int32             number_of_samples;
	__int32             units_per_millivolt;
	
	unsigned short      ecg_data_block_size;    //  size of ECG data block 0 - (raw ecg data no blocks no compression)
	unsigned short      number_of_leads;
	char                leads_name_array[MAX_LEAD_COUNT][LEAD_NAME_LENGTH]; // ascii names of leads
	char                device_class[DEVICE_ID_NAME_LENGTH];
	char                device_number[DEVICE_ID_NAME_LENGTH];
	char                record_id[SHA1_ID_LENGTH]; // "Rec" + sha1
	char                device_id[SHA1_ID_LENGTH]; // "Dev" + sha1

	unsigned short      ecg_data_crc;           // crc16(initial 0xffff)
	unsigned short      time_index_table_crc;   // crc16(initial 0xffff)

	unsigned short      primary_events_crc;     // crc16(initial 0xffff)
	unsigned short      user_def_data_crc;      // crc16(initial 0xffff)

	int                 electrode_placement_id; // possible values in lead_system.h
	unsigned short      header_crc;             // crc16(initial 0xffff)

} beecardia_file_header_st;
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	unsigned short	crc_ccitt;	// CRC-CCITT from byte 2 up to the end of the ECG block.
	short			block_size;
	__int32			block_number;
	__int32			start_sample_number;
	short			samples_in_block;
	char			number_of_channels;
	char			data_packing_method; // data packing method
	short			lead_off_status;
} ecg_block_header_st; 				// 18 bytes long 18/512 = 3.5% overhead
#pragma pack(pop)   /* restore original alignment from stack */
/////////////////////////////////////////////////////////////////////////////

#define DATA_FORMAT_RAW16     1
#define DATA_FORMAT_MIT212    2
#define DATA_FORMAT_DDT12     3
#define DATA_FORMAT_RAW24     4
#define DATA_FORMAT_DCT       5 // 5 - lossy, discrete cosines transform based compression
#define DATA_FORMAT_DDT16     6
#define DATA_FORMAT_RAW32     7
/////////////////////////////////////////////////////////////////////////////

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)     /* set alignment to 2 byte boundary */
typedef struct {
	bee_data_time_st record_start_time;
	__int32 samples_count;                // total samples in record
	__int32 sampling_rate;                //  [Hz].
	__int32 units_per_millivolt;
	unsigned short wake_up_timeout;
	short ecg_data_block_size;            //  size of ECG data block
	short number_of_leads;
	char leads_identification_array[16];
	short storage_format;
} primary_record_info_st;
#pragma pack(pop)   /* restore original alignment from stack */
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	unsigned short lead_system_id;
	unsigned char  WCT1; // Wilson Central Terminal and Augmented Lead Control Register;
	unsigned char  WCT2; // Wilson Central Terminal Control Register
} leads_identification_st;
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	unsigned short lead_system_id;
	unsigned short sampling_rate_id;
	unsigned short units_per_mv;
	unsigned short number_of_channels;
	unsigned short ac_filter_mode;
	unsigned short sampling_rate_val;
	__int32 sampling_rate_ppm;
	unsigned short number_of_wires;
} ecg_parameters_st;
/////////////////////////////////////////////////////////////////////////////

#pragma pack(push)  /* push current alignment to stack  */
#pragma pack(2)     /* set alignment to 2 byte boundary */
typedef struct {
	char device_state;           // DEVICE_STATUS_BIT_DATA_STORING_MODE  DEVICE_STATUS_BIT_MONITORING_MODE,
	                             // DEVICE_STATUS_BIT_MEMORY_READ, DEVICE_STATUS_BIT_MEMORY_NOT_EMPTY
// device_status -> to device_state
	char record_stop_cause;
	// memory_state
	short current_voltage;       // millivolts
	short max_voltage;           // max millivolts battery
	short min_voltage;	         // min millivolts battery
	__int32 k_bytes_stored;      // user kByte flash
	__int32 k_bytes_available;   // max kByte flash
	__int32 start_recorded_page;
	__int32 last_recorded_page;
	unsigned short  patient_event_count;
	unsigned short  m32_ID0;
	unsigned short  size_shifter;
	short bt_pause_flag; // incremented by every pause request, decremented by resume request, BT continued when flag is 0
	unsigned long bt_off_timeout;
	unsigned long bt_wakeup_timeout;

} device_status_st;
#pragma pack(pop)   /* restore original alignment from stack */
/////////////////////////////////////////////////////////////////////////////


typedef struct {
	short error_mask;
	char  error_id[4];
} device_error_st;
/////////////////////////////////////////////////////////////////////////////

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)     /* set alignment to 2 byte boundary */
typedef struct {
	unsigned short	crc;
	unsigned char	opcode;
	unsigned char	message_number;
	unsigned short	data_length;
} command_header_st;

typedef struct {
	unsigned char	net_prefix[6];
	unsigned short	crc;
	unsigned char	opcode;
	unsigned char	message_number;
	unsigned short	data_length;
} message_header_st;

#pragma pack(pop)   /* restore original alignment from stack */
#define COMMAND_HEADER_SIZE	6
/////////////////////////////////////////////////////////////////////////////

typedef	struct {
	char opcode;
	char command;
	char value[20];
} bee_upload_st;
/////////////////////////////////////////////////////////////////////////////

typedef	struct {
	long	end_page;	
	long	start_page;	
} record_blocks_size_st;
/////////////////////////////////////////////////////////////////////////////

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 2 byte boundary */
typedef struct {
  char sample_rate_registr_value;
  unsigned char wct1;
  unsigned char wct2;
  unsigned char inv;
  unsigned char rld_sensp;
  unsigned char rld_sensn;
} leads_system_st;
#pragma pack(pop)   /* restore original alignment from stack */
/////////////////////////////////////////////////////////////////////////////

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(2)
typedef struct {
	short event_id;
	short event_param;
	__int32 event_start_sample;
} PRIMARY_EVENT;
#pragma pack(pop)
/////////////////////////////////////////////////////////////////////////////
typedef struct {
	__int32 event_start_sample;
	__int32 event_id;
	__int32 cx;
	__int32 event_param;
} PRIMARY_EVENT_16;
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	DsDeviceId dsDeviceId;
	leads_identification_st lead_id;
	device_status_st device_state;
	
	primary_record_info_st record_info;
	
	char hash_fio[DEVICE_ID_NAME_LENGTH];
	char firmvare_version[VERSION_NAME_LENGTH];
	char unread_record;
	leads_identification_st leads_identification;
	short sampling_rate_id;
	short voltmeter_calibration_value;
} bee_device_info_st;
/////////////////////////////////////////////////////////////////////////////

#define PRIM_EVENTS_SIZE            8192
#define PRIM_EVENTS_MAX_COUNT       (PRIM_EVENTS_SIZE / (sizeof(PRIMARY_EVENT)))
/////////////////////////////////////////////////////////////////////////////


#endif // _BEE_DATA_TYPES_H_
