// Helps keyboard code not wrap
#define KBP NKROKeyboard.press
#define KBR NKROKeyboard.release

//// Board specific
// This is where pins should be specified
// This should also use

// Pins for Trinket-based 2/4k keypads
#if defined (ADAFRUIT_TRINKET_M0) && ! defined (TOUCH)
const uint8_t pins[] = { 0, 2, 20, 19, 3 };
#define NPPIN 1
#define TPIN 4
#endif

// Pins for Trinket-based 2k touch keypad
#if defined (ADAFRUIT_TRINKET_M0) && defined (TOUCH)
const uint8_t pins[] = { 3, 4, 1 };
#define NPPIN 0
#define TPIN 1
#endif

// Pins for all newer models
#ifdef SAMD21MINI
const uint8_t pins[] = { 12, 11, 10, 9, 6, 5, 0 };
#define NPPIN 13
#define TPIN A0
#define Serial SerialUSB
#endif

//// Platform specific

// Platform specific
#if defined (__AVR_ATmega32U4__)
#include <EEPROM.h>
const uint8_t pins[] = { 2, 3, 7, 9, 10, 11, 12, 4 };
#define NPPIN 5
#define COMMIT
// If not using a 32u4, it will be a SAMD board
#else
#include <FlashAsEEPROM.h>
#include <Adafruit_FreeTouch.h>
#define COMMIT EEPROM.commit();
Adafruit_FreeTouch qt = Adafruit_FreeTouch(TPIN, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
#endif


//// Model specific

// The 7K uses different mapping
#if numkeys == 7
const float gridMap[] = {0,1,2,3,4,5,2.5};
uint8_t mapping[][7] = {
{115,100,102,106,107,108,32},
{136,126,173,172,175,174,0}
};
// Set keys past 2 even if only 2 exist
#else
const float gridMap[] = {0,1,2,3};
uint8_t mapping[][4] = {
{122,120,99,118},
{136,126,173,172}
};
#endif
