#define _VERSION 81 // 10.08.2024 - 1800 W
#define _XTAL_FREQ 40000000 // 10MHz * 4 (PLL) = 40MHz

#include "xc.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

// PIC18F46K80 Configuration Bits
#pragma config FOSC = HS2       // High-Speed crystal oscillator
#pragma config PLLCFG = ON      // 4x PLL enabled (40 MHz clock)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor disabled
#pragma config IESO = OFF       // Two-Speed Start-up disabled
#pragma config PWRTEN = ON      // Power-up Timer enabled
#pragma config BOREN = ON       // Brown-out Reset enabled
#pragma config BORV = 1         // BOR Voltage 2.7V
#pragma config BORPWR = HIGH    // High power BOREN
#pragma config WDTEN = ON       // Watchdog Timer enabled
#pragma config WDTPS = 128      // Watchdog Postscaler 1:128
#pragma config STVREN = ON      // Stack Overflow Reset enabled
#pragma config XINST = OFF      // Extended Instruction Set disabled
#pragma config SOSCSEL = DIG    // Secondary oscillator pins RC0/RC1 as digital I/O
#pragma config MSSPMSK = MSK7

// Default EEPROM calibration data
__EEPROM_DATA(0xA4,0x06,0xFA,0x00,0xC2,0x01,0xF4,0x01); // 0...7
__EEPROM_DATA(0x41,0x00,0x2D,0x00,0xC8,0x00,0x64,0x00); // 8...15
__EEPROM_DATA(0x32,0x00,0x0A,0x05,0x05,0x0A,0x0A,0x00); // 16...23
__EEPROM_DATA(0x01,0x01,0x00,0x01,0x00,0x00,0x00,0x00); // 24...31

#include "macros.h"

// Global variables
volatile UINT  frwd, refl, volt, volt_view, curr, temp, freq;
volatile UINT  volt_old = 0, temp_old = 0, fpeak = 0, rpeak = 0, cpeak = 0;
volatile UINT  fpeak_old = 0, rpeak_old = 0, cpeak_old = 0;
volatile UINT  max_pwr, max_ref, max_volt, max_curr, max_temp, max_tFAN;
volatile UINT  tempM = 0, voltM = 0, elpf = 0, lpferrP, lpferrC, lpferrR;
volatile UINT  frtout = 0, cvtout = 0;
volatile UCHAR ptt = 0, bypass = 0, lcd = 0, reldel, biasdel, fanptt, maxf = 0, fan = 0, med = 0;
volatile UCHAR band, band_old = 0, bstate, bstate_old = 0, err = 0, err_old = 0xFF, aband;
volatile UCHAR hystT, hystF, beeper, logo, sensor, ptt_hold, ptt_htimer = 0;
volatile UCHAR buffRX[_MAX_USART_RX], buffTX[_MAX_USART_TX], rxb, txb, rx_end, tx_len;
volatile ULONG tmp;

#include "eep.h"
#include "usart.h"
#include "digital.h"
#include "adc.h"
#include "nextion.h"

void main(void)
{
    di();
    STKPTR = 0x00;

    // Disable analog comparators
    CM1CON = 0x00;
    CM2CON = 0x00;
    CCP1CON = 0x00;

    // Pin mode: 0 = analog input, 1 = digital I/O
    // AN0(RA0), AN1(RA1), AN3(RA3), AN4(RA5) are analog; AN2 is digital
    ANCON0 = 0b11100100;
    // AN5(RE0) is analog; others digital
    ANCON1 = 0b11111110;

    // ADC setup: 10-bit compatibility mode (BTM=0), VREF+ = VDD, VREF- = VSS
    ADCON1 = 0b00001000;
    // Right justified, 4 TAD acquisition time, Fosc/64 conversion clock
    ADCON2 = 0b10010110;

    // Port directions (TRIS)
    TRISA = 0b00111111;
    TRISB = 0b11000000;
    TRISC = 0b10000001; // RC7(RX1)=in, RC0(T1CKI)=in
    TRISD = 0b00000000;
    TRISE = 0b00000001; // RE0(AN5)=in

    // Reset output latches
    LATA = 0x00; LATB = 0x00; LATC = 0x00; LATD = 0x00; LATE = 0x00;

    // EUSART1 configuration (Nextion LCD, 115200 baud at 40 MHz)
    RCSTA1 = 0b10010000; // SPEN=1, CREN=1
    TXSTA1 = 0b00100100; // TXEN=1, BRGH=1
    BAUDCON1bits.BRG16 = 0;
    SPBRG1 = 21;         // 115200 baud

    // Timers setup
    T0CON = 0b10000000;  // TMR0 on, 16-bit
    T1CON = 0b10000111;  // TMR1 on, external clock on RC0/T1CKI
    T3CON = 0b10110001;  // TMR3 on, ~50 ms

    // Watchdog and pull-ups
    WDTCON = 0x1;
    INTCON2bits.RBPU = 1;

    // Interrupts setup
    PIR1bits.RC1IF = 0;
    PIE1bits.RC1IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    EEsetupRead();
    EEmbandRead();

    // Show logo page
    if (logo)
    {
        if (!LCDpageSelect(_PAGE_LOGO)) { _FULL_RESET; }
        DelayS(3);
    }
    else 
    {
        DelayS(1);
    }

    // Show main page
    if (!LCDpageSelect(_PAGE_MAIN)) { _FULL_RESET; }
    LCDversionSend();

    SetBand(); 
    Beep();

    while(1)
    {
        ADCmeasure();
        ProtectCheck();
        SlowControl();
        SendStatusToLCD();
        ReadStatusFromLCD();
    }
}