/*
Copyright (C) 2013
*/

#ifndef _BEE_DEV_INFO_H_
#define _BEE_DEV_INFO_H_


#define DEVICE_NAME_ECG                   "MobiCardio"
#define DEVICE_NAME_SPIRO                 "MobiSpiro"
#define DEVICE_NAME_STETHO                "MobiStetho"

#ifdef  ECG
#define DEVICE_NAME_DEFAULT               DEVICE_NAME_ECG
#elif   SPIRO
#define DEVICE_NAME_DEFAULT               DEVICE_NAME_SPIRO
#elif   STETHO
#define DEVICE_NAME_DEFAULT               DEVICE_NAME_STETHO
#else
#define DEVICE_NAME_DEFAULT               "Mobi___"
#endif

#define DEVICE_NUMBER_DEFAULT             "0001"

#endif // _BEE_DEV_INFO_H_
