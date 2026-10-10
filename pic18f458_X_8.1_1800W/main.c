//*********************************************************************************
//#define _VERSION 61 //17.07.2022
//#define _VERSION 62 //22.07.2022
//#define _VERSION 63 //23.09.2022
//#define _VERSION 70 //30.09.2022
//#define _VERSION 71 //04.10.2022
//#define _VERSION 80 //10.11.2022
#define _VERSION 81 //10.08.2024 - 1800 W
//*********************************************************************************
#define _XTAL_FREQ 40000000 //10MHz*4(PLL)
#include <xc.h>
#include <string.h>
#include <math.h>
//
#pragma config OSC = HSPLL, OSCS = OFF
#pragma config PWRT = ON, BORV = 27, BOR = ON
#pragma config WDT = ON, WDTPS = 128
#pragma config STVR = ON, LVP = OFF, DEBUG = OFF
#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF
#pragma config CPB = OFF, CPD = OFF
#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF
#pragma config WRTC = OFF, WRTB = OFF, WRTD = OFF
#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF
#pragma config EBTRB = OFF
//
//forward 1700
//reflected 250
//curr 45.0
//volt 50.0
//temp 65.0
//fan temp 45.0
//lpf error power 200
//lpf error curr 10.0
//lpf reaction 50
//ptt hold time 0
//temp hyst 10
//fan hyst 5
//rel delay 5
//bias delay 10
//t sensor calibration 10 [0]
//beep - on
//logo - on
//band 1.8
//auto band - on
//
__EEPROM_DATA(0xA4,0x06,0xFA,0x00,0xC2,0x01,0xF4,0x01);//0...7
__EEPROM_DATA(0x41,0x00,0x2D,0x00,0xC8,0x00,0x64,0x00);//8...15
__EEPROM_DATA(0x32,0x00,0x0A,0x05,0x05,0x0A,0x0A,0x00);//16...23
__EEPROM_DATA(0x01,0x01,0x00,0x01,0x00,0x00,0x00,0x00);//24...31
//
#include "macros.h"
//*********************************************************************************
volatile near UINT  frwd, refl, volt, volt_view, curr, temp, freq;
volatile near UINT  volt_old = 0, temp_old = 0, fpeak = 0, rpeak = 0, cpeak = 0;
volatile near UINT  fpeak_old = 0, rpeak_old = 0, cpeak_old = 0;
volatile near UINT  max_pwr, max_ref, max_volt, max_curr, max_temp, max_tFAN;
volatile near UINT  tempM = 0, voltM = 0, elpf = 0, lpferrP, lpferrC, lpferrR;
volatile near UINT  frtout = 0, cvtout = 0;
volatile near UCHAR ptt = 0, bypass = 0, lcd = 0, reldel, biasdel, fanptt, maxf = 0, fan = 0, med = 0;
volatile near UCHAR band, band_old = 0, bstate, bstate_old = 0, err = 0, err_old = 0xFF, aband;
volatile near UCHAR hystT, hystF, beeper, logo, sensor, ptt_hold, ptt_htimer = 0;
volatile near UCHAR buffRX[_MAX_USART_RX], buffTX[_MAX_USART_TX], rxb, txb, rx_end, tx_len;
volatile near ULONG tmp;
//*********************************************************************************
#include "eep.h"
#include "usart.h"
#include "digital.h"
#include "adc.h"
#include "nextion.h"
//*********************************************************************************
void main(void)
{
	di();
	STKPTR = 0x00;
	//comparator off
	CMCON = 0x07;
    CCP1CON = 0x00;
    //adc
	ADCON0 = 0b10000000;
	ADCON1 = 0b11001001;//FOSC/64 TAD = 1.6uS
	//I/O
	TRISA = 0b00111111;
	TRISB = 0b11000000;
	TRISC = 0b10000001;
	TRISD = 0b00000000;
	TRISE = 0b00000001;
	//reset PORTS
	PORTA = 0x00; PORTB = 0x00; PORTC = 0x00; PORTD = 0x00; PORTE = 0x00; 
	LATA = 0x00; LATB = 0x00; LATC = 0x00; LATD = 0x00; LATE = 0x00;
	//USART
	RCSTA = 0b10010000;//SP enable
	TXSTA = 0b00100100;//TXEN = 1 / BRGH = 1
	SPBRG = 21;//115200 bod
	//TIMER0
	T0CON = 0b10000000;
    //TIMER1
    T1CON = 0b10000111;//external clock
    //TIMER3
    T3CON = 0b10110001; //~50 ms
	//WDT on
	WDTCON = 0x1;
	//pull-up resistors off
	INTCON2bits.RBPU = 1;
	//Interrupts on
	PIE1bits.RCIE = 1;
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
    //
	EEsetupRead();
    EEmbandRead();
	//logo page -----------------------------
    if (logo)
    {
        if (!LCDpageSelect(_PAGE_LOGO)) {_FULL_RESET;}
        DelayS(3);
    }
    else DelayS(1);
    //main page
    if (!LCDpageSelect(_PAGE_MAIN)) {_FULL_RESET;}
    //version send
    LCDversionSend();
	//
	SetBand(); 
    Beep();
    //---------------------------------------
	while(1)
	{
		ADCmeasure();
		ProtectCheck();
        //
        SlowControl();
		//
		SendStatusToLCD();
		ReadStatusFromLCD();
	}
}
//*********************************************************************************

