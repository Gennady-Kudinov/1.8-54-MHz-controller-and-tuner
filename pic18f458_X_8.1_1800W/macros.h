#ifndef _MACROS_H
#define _MACROS_H

#include "xc.h"

#define UCHAR       unsigned char
#define SCHAR       signed char
#define UINT        unsigned int
#define SINT        signed int
#define ULONG       unsigned long
#define SLONG       signed long
#define FLOAT       float
#define DOUBLE      double

#define _NOP            asm("nop")
#define _WDT_RESET      asm("clrwdt")
#define _FULL_RESET     asm("reset")

#define _MAX_USART_RX   10
#define _MAX_USART_TX   64

// Direct bit-read of RA4 pin for PTT input
#define _PTT_INPUT      ((PORTA >> 4) & 1)

// Relays and sequencer outputs
#define _RELAY_INPUT    LATCbits.LATC4
#define _RELAY_OUTPUT   LATBbits.LATB3
#define _BIAS           LATEbits.LATE2
#define _POWER_VDS      LATEbits.LATE1

// LPF Band outputs
#define _BAND_35        LATDbits.LATD0
#define _BAND_710       LATDbits.LATD1
#define _BAND_1418      LATDbits.LATD2
#define _BAND_2128      LATDbits.LATD3

// Fan and buzzer outputs
#define _FAN_SPEED      LATCbits.LATC1
#define _BEEPER         LATCbits.LATC5

// Power coefficients
#define _PWR_COEFF      550 // 1023*1023 / 550 = 1900 Watt
#define _MIN_FRWD_HOLD  520 // 500W

// EEPROM memory map
#define _EE_PWR         0
#define _EE_REF         2
#define _EE_CUR         4
#define _EE_VOL         6
#define _EE_TMP         8
#define _EE_TFN         10
#define _EE_LPF_P       12
#define _EE_LPF_C       14
#define _EE_LPF_R       16
#define _EE_HOLD        17
#define _EE_HYSTT       18
#define _EE_HYSTF       19
#define _EE_RDEL        20
#define _EE_BDEL        21
#define _EE_SENS        22
#define _EE_FPTT        23
#define _EE_BEEP        24
#define _EE_LOGO        25
#define _EE_BAND        26
#define _EE_ABAND       27
#define _EE_BYPASS      28

// Error bit masks
#define _ERROR_PWR      0x01
#define _ERROR_REF      0x02
#define _ERROR_CUR      0x04
#define _ERROR_PTT      0x08
#define _ERROR_LPF      0x10
#define _ERROR_TMP      0x20
#define _ERROR_VOL      0x40

// LCD page IDs
#define _PAGE_MAIN      0xA0
#define _PAGE_LOGO      0xA1
#define _PAGE_SETUP     0xA2

#endif