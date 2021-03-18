/*
 * Copyright (c) 2013, Beecardia firmware.
 * All rights reserved.
 */

#ifndef _BEE_COMMANDS_CODES_H_
#define _BEE_COMMANDS_CODES_H_

#define RESPONSE_OK                     0
#define RESPONSE_UNKNOWN_COMMAND        1
#define RESPONSE_BAD_CRC                2
#define RESPONSE_LENGTH_ERROR           3
#define RESPONSE_DEAD_LEDS_ERROR        4
#define RESPONSE_TIMEOUT                32
#define DUPLEX_TIMEOUT                  33
#define ERASE_NOT_READ                  34
#define RESPONSE_FILENAME               35
#define RESPONSE_NOT_MONITORING_MODE    36
#define RESPONSE_SOCKET_ERROR           37

#define DISCONNECT_ERROR_TIMEOUT        20 /* seconds */

#define CMD_DEFAULT_RESPONSE            0xfa
#define CMD_SET_DEVICE_ID               0x10
#define CMD_GET_DEVICE_ID               0x11
#define CMD_DEVICE_ID                   0x12

#define CMD_GET_FIRMWARE_VERSION        0x13
#define CMD_FIRMWARE_VERSION            0x14

#define CMD_SET_BEE_DATE_TIME           0x15
#define CMD_GET_BEE_DATE_TIME           0x16
#define CMD_BEE_DATE_TIME               0x17

#define CMD_GET_DEVICE_STATUS           0x18
#define CMD_DEVICE_STATUS               0x19

#define CMD_SET_ECG_CALIBRATION_VALUES	0x1a
#define CMD_GET_ECG_CALIBRATION_VALUES	0x1b
#define CMD_ECG_CALIBRATION_VALUES      0x1c

#define CMD_SET_VOLTMETER_CLB_VALUES    0x1d
#define CMD_GET_VOLTMETER_CLB_VALUES    0x1e
#define CMD_VOLTMETER_CLB_VALUES        0x1f

#define CMD_SET_MONITORING_MODE         0x20
#define CMD_MONITORING_MODE             0x21
#define CMD_ECG_DATA_BLOCK              0x22
#define CMD_DATA_BLOCK                  0x22
//#define CMD_RAW_DATA                    0x23

#define CMD_SET_OSC_VALIDATE            0x24
#define CMD_OSC_VALIDATE                0x25

#define CMD_SET_LEAD_SYSTEM             0x26
#define CMD_GET_LEAD_SYSTEM             0x27
#define CMD_LEAD_SYSTEM                 0x28

#define CMD_SET_WAKE_UP_TIMEOUT         0x29
#define CMD_GET_WAKE_UP_TIMEOUT         0x2a
#define CMD_WAKE_UP_TIMEOUT             0x2b

#define CMD_SET_MONITORING_TIME         0x30
#define CMD_GET_MONITORING_TIME         0x31
#define CMD_MONITORING_TIME             0x32

#define CMD_GET_BOOKMARK                0x34
#define CMD_BOOKMARK                    0x35
#define CMD_HOLTER_BOOKMARK             0x36

#define CMD_READ_BLOCK_RECORDING        0x3e
#define CMD_BLOCK_RECORDING             0x3f

#define CMD_GET_PRIMARY_INFO            0x3a
#define CMD_PRIMARY_INFO                0x3b

#define CMD_GET_DEVICE_MEMORY           0x3c

#define CMD_START_ECG_RECORDING	        0x40
#define CMD_STOP_ECG_RECORDING          0x41
#define CMD_ECG_RECORDING               0x42
#define CMD_START_DELAYED_RECORDING     0x37

#define CMD_GET_DEVICE_ERROR            0x43
#define CMD_DEVICE_ERROR                0x44

#define CMD_GET_LEADS_IDENTIFICATION    0x45
#define CMD_SET_LEADS_IDENTIFICATION    0x46
#define CMD_LEADS_IDENTIFICATION        0x47

#define	CMD_ERASE_ALL                   0x48
#define	CMD_GET_ERASE                   0x52
#define CMD_LOGGER                      0x4e
#define CMD_GET_LOGGER                  0x4f

#define CMD_SET_HASH_FIO                0x55
#define CMD_GET_HASH_FIO                0x56
#define CMD_HASH_FIO                    0x57

#define CMD_GET_UNREAD_RECORD           0x50
#define CMD_UNREAD_RECORD               0x51

#define CMD_SET_MAX_MIN_VOLTMETER       0x58
#define CMD_MAX_MIN_VOLTMETER           0x59

#define CMD_READ_ECG_RECORDING          0x60
#define CMD_READ_RECORDING              0x61
#define CMD_READ_STOP_RECORDING         0x62
#define CMD_STOP_RECORDING              0x63

#define CMD_HOLTER_RECORDING            0x66

#define CMD_GET_ADC_SAMPLING_RATE_ID    0x67
#define CMD_SET_ADC_SAMPLING_RATE_ID    0x68
#define CMD_ADC_SAMPLING_RATE_ID        0x69

#define CMD_GET_ADS_REGISTERS           0x6a
#define CMD_SET_ADS_REGISTERS           0x6b
#define CMD_ADS_REGISTERS               0x6c
#define CMD_SAVE_ADS_REGISTERS          0x6d

#define CMD_SET_UPLOAD_MODE             0x71
#define CMD_ENTER_FLY_MODE              0x72

#define CMD_GET_ECG_PARAMETERS          0x73
#define CMD_ECG_PARAMETERS              0x75

#define CMD_SET_AC_FILTER_ID            0x76
#define CMD_AC_FILTER_ID                0x77

#define CMD_SET_UPLOAD_BLUETOOTH_MODE   0x80

#endif // _BEE_COMMANDS_CODES_H_
