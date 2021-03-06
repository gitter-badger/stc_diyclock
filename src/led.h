// LED functions for 4-digit seven segment led

#include <stdint.h>


// index into ledtable[]
#define LED_a   0xa
#define LED_b   0xb
#define LED_c   0xc
#define LED_d   0xd
#define LED_e   0xe
#define LED_f   0xf
#define LED_BLANK  0x10
#define LED_DASH   0x11
#define LED_h   0x12
#define LED_dp  0x13

void filldisplay(uint8_t pos, uint8_t val, __bit dp);
