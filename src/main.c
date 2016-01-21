//
// STC15F204EA blinky example
// inspired by http://jjmz.free.fr/?p=179 http://jjmz.free.fr/?p=191
// and stc15f204ea datasheet
//

#include <stc12.h>
#include <stdint.h>
#include <stdio.h>
#include "adc.h"
#include "ds1302.h"
#include "led.h"
#include "./soft_serial/serial.h"
    
#define FOSC    11059200

#define WDT_CLEAR()    (WDT_CONTR |= 1 << 4) // clear wdt

#define DEBUG   0

/* ------------------------------------------------------------------------- */
/* Printing functions */
/* ------------------------------------------------------------------------- */

#define printf printf_fast     // see sdcc user guide

#define RELAY   P1_4
#define BUZZER  P1_5
    
#define ADC_LIGHT 6
#define ADC_TEMP  7

#define SW2     P3_0
#define S2      1
#define SW1     P3_1
#define S1      0

#define PRESS_NONE   0
#define PRESS_SHORT  1
#define PRESS_LONG   2

enum {
    M_NORMAL,
    M_SET_HOUR,
    M_SET_MINUTE,
    M_TEMP_DISP,
    M_TEMP_ADJUST,
    M_DATE_DISP,
    M_SET_MONTH,
    M_SET_DAY,
    M_WEEKDAY_DISP,
    M_SET_WEEKDAY
} display_mode;

/* ------------------------------------------------------------------------- */

void _delay_ms(uint8_t ms)
{	
    // i,j selected for fosc 11.0592MHz, using oscilloscope
    // the stc-isp tool gives inaccurate values (perhaps for C51 vs sdcc?)
    // max 255 ms
    uint8_t i, j;
    do {
    	i = 4;
    	j = 200;
    	do
    	{
    		while (--j);
    	} while (--i);
    } while (--ms);
}

// GLOBALS
uint8_t i;
int count = 0;
const char* startstring = "DIY Clock starting.\n";
unsigned int tempval = 0;    // temperature sensor value
uint8_t lightval = 0;   // light sensor value
volatile uint8_t displaycounter = 0;
struct ds1302_rtc rtc;

uint8_t display[4] = {0,0,0,0};     // led display buffer
__bit  display_colon = 0;         // flash colon
__bit  flash_hours = 0;
__bit  flash_minutes = 0;
uint8_t dmode = M_NORMAL;   // display mode state

volatile uint8_t debounce[2] = {0xFF,0xFF};      // switch debounce buffer
volatile uint8_t switchcount[2] = {0,0};

/* Timer0 ISR */
void tm0_isr() __interrupt 1 __using 1
{
    // led display refresh cycle
    // select one of four digits
    uint8_t digit = displaycounter % 4; 

    // turn off all digits, set high    
    P3 |= 0x3C;

    // auto dimming, skip lighting for some cycles
    if (displaycounter > 247  || displaycounter > lightval) {
        // fill digits
        P2 = display[digit];
        // turn on selected digit, set low
        P3 &= ~((0x1 << digit) << 2);  
    }
    displaycounter++;
    // done    
    
    // debouncing stuff
    if (displaycounter == 0) {
        if (debounce[0] == 0)
            switchcount[0]++;
        if (debounce[1] == 0)
            switchcount[1]++;
    }
    // read sw
    debounce[0] = (debounce[0] << 1) | SW1;
    debounce[1] = (debounce[1] << 1) | SW2;  

}

void Timer0Init(void)		//10ms@11.0592MHz
{
    AUXR &= 0x7F;		//Timer clock is 12T mode
    TMOD &= 0xF0;		//Set timer work mode
    TL0 = 0xB0;		//Initial timer value
    TH0 = 0xFF;		//Initial timer value
    TF0 = 0;		//Clear TF0 flag
    TR0 = 1;		//Timer0 start run
    ET0 = 1;
}

uint8_t getkeypress(uint8_t keynum)
{
    uint8_t retval = PRESS_NONE;
    if (switchcount[keynum] > 3)
        if (switchcount[keynum] > 128)
            retval = PRESS_LONG;
        else
            retval = PRESS_SHORT;
    if (debounce[keynum] != 0)
        // reset switch count only when released
        switchcount[keynum] = 0;
    else
        retval = PRESS_NONE; // dont register keypress yet
    return retval;
}

/*********************************************/
int main()
{
    // SETUP
    // set ds1302, photoresistor, & ntc pins to open-drain output, already have strong pullups
    P1M1 |= (1 << 0) | (1 << 1) | (1 << 2) | (1<<6) | (1<<7);
    P1M0 |= (1 << 0) | (1 << 1) | (1 << 2) | (1<<6) | (1<<7);
    
	_delay_ms(100);
      
    /* init the software uart */
    UART_INIT();

    /* simple greeting message */
    printf("%s", startstring);
    
    RELAY = 1;
    //BUZZER = 1;
        
    // init rtc
    ds_writebyte(DS_ADDR_WP, 0); // clear WP (Write Protect)
    ds_writebyte(DS_ADDR_SECONDS, 0); // clear CH (Clock Halt), start clock
    
    Timer0Init(); // display refresh
    
    // LOOP
    while(1)
    {             
      RELAY = 0;
      //BUZZER = 0;
      _delay_ms(150);

      RELAY = 1;
      //BUZZER = 1;
      //printf("counter: %d \n", count);
      
      lightval = getADCResult8(ADC_LIGHT);
      tempval = getADCResult(ADC_TEMP);
      
      ds_readburst((uint8_t *) &rtc); // read rtc

      switch (dmode) {
          case M_SET_HOUR:
              display_colon = 1;
              flash_hours = !flash_hours;
              if (getkeypress(S2)) {
                // TODO: incr hour
              }
              if (getkeypress(S1))
                  dmode = M_SET_MINUTE;
              break;
          case M_SET_MINUTE:
              flash_hours = 0;
              display_colon = 1;
              flash_minutes = !flash_minutes;
              if (getkeypress(S2)) {
                  // TODO: incr minutes
              }
              if (getkeypress(S1))
                  dmode = M_NORMAL;
              break;
          case M_TEMP_DISP:
              // TODO: display temp
              break;
          case M_TEMP_ADJUST:
              // TODO: adjust temp
              break;
          case M_DATE_DISP:
              // TODO: display date
              break;
          case M_SET_MONTH:
              // TODO: set month
              break;
          case M_SET_DAY:
              // TODO: set day of month
              break;
          case M_WEEKDAY_DISP:
              // TODO: display day of week
              break;
          case M_SET_WEEKDAY:
              // TODO: set day of week
              break;
          case M_NORMAL:
          default:
              flash_hours = 0;
              flash_minutes = 0;
              display_colon = count & 0x1; // alternate flashing colon
              if (getkeypress(S1))
                  dmode = M_SET_HOUR;         
      };

      
      if (flash_hours) {
          filldisplay(display, 0, 10, 0);
          filldisplay(display, 1, 10, display_colon);
      } else {
          filldisplay(display, 0, rtc.h12.tenhour, 0);
          filldisplay(display, 1, rtc.h12.hour, display_colon);      
      }
      
      if (flash_minutes) {
          filldisplay(display, 2, 10, display_colon);
          filldisplay(display, 3, 10, 0);  
      } else {
          filldisplay(display, 2, rtc.tenminutes, display_colon);
          filldisplay(display, 3, rtc.minutes, 0);  
      }
      
#ifdef DEBUG
      if (display_colon) {
          // only print every second
          printf("adc raw: L: %03d, T: %03d\n", lightval, tempval);   
          printf("yy mm dd hh mm ss am/pm 24/12 ww \n%d%d %d%d %d%d %d%d %d%d %d%d     %d     %d  %d\n",
              rtc.tenyear, rtc.year, rtc.tenmonth, rtc.month, rtc.tenday, rtc.day, rtc.h12.tenhour, rtc.h12.hour, 
              rtc.tenminutes, rtc.minutes, rtc.tenseconds, rtc.seconds, rtc.h12.pm, rtc.h12.hour_12_24, rtc.weekday);

      printf("switch, count: %d, %d - %d, %d\n", S1, getkeypress(S1), S2, getkeypress(S2));      
      }
#endif
      
            
      _delay_ms(200);
      count++;
      WDT_CLEAR();
    }
}
/* ------------------------------------------------------------------------- */
