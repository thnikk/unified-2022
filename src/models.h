// Helps keyboard code not wrap
#define KBP NKROKeyboard.press
#define KBR NKROKeyboard.release

//// Board specific
// This is where pins should be specified
// This should also use

// Pins for Trinket-based 2/4k keypads
#if defined (QTPY)
const uint8_t pins[] = { 0, 1, 2, 4, 5 };
#define NPPIN 3
#elif defined (ADAFRUIT_TRINKET_M0) && ! defined (TOUCH)
const uint8_t pins[] = { 0, 2, 20, 19, 3 };
#define NPPIN 1
#endif


// Dotstar for all trinket-based models
#if defined (ADAFRUIT_TRINKET_M0) || defined (QTPY)
CRGB ds[1];
#endif

// Pins for all newer models
#ifdef SAMD21MINI
// Side button is always last value in the array.
const uint8_t pins[] = { 12, 11, 10, 9, 6, 5, A3, A0 };
#define NPPIN 13
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
#endif


// Pins for Trinket-based 2k touch keypad
#if defined (ADAFRUIT_TRINKET_M0) || defined (QTPY)
#if defined (TOUCH)
const uint8_t pins[] = { 3, 4, 1 };
#define NPPIN 0
#define TPIN 1
Adafruit_FreeTouch qt = Adafruit_FreeTouch(1, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_1 = Adafruit_FreeTouch(3, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_2 = Adafruit_FreeTouch(4, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
#elif defined (ADAFRUIT_TRINKET_M0) || defined (QTPY)
#if ! defined (TOUCH)
Adafruit_FreeTouch qt = Adafruit_FreeTouch(4, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
#endif
#elif defined (SAMD21MINI)
Adafruit_FreeTouch qt = Adafruit_FreeTouch(A0, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
#endif
#endif

#if defined (TOUCH)
    #define CHECK checkTouch();
#else
    #define CHECK checkSwitch();
#endif


//// Mapping nicknames
// These are only used by the compiler so keys can have nicknames during assignment
#define SK_LEFT  173
#define SK_RIGHT 172
#define SK_UP    175
#define SK_DOWN  174
#define SK_Z     122
#define SK_X     120
#define SK_C     99
#define SK_V     118
#define SK_S     115
#define SK_D     100
#define SK_F     102
#define SK_J     106
#define SK_K     107
#define SK_L     108
#define SK_SP    32
#define SK_BKTK  96
#define SK_ESC   136
#define SK_F13   149
#define SK_F14   150
#define SK_F15   151
#define SK_F16   152
#define SK_F17   153
#define SK_F18   154
#define SK_F19   155
#define SK_F20   156
#define SK_F21   157
#define SK_F22   158
#define SK_F23   159
#define SK_F24   160
#define SK_MB1   195
#define SK_MB2   196
#define SK_MB3   197
#define SK_MB4   198
#define SK_MB5   199

//// Model specific

// The 7K uses different mapping
#if numkeys == 7
const float gridMap[] = {0,1,2,3,4,5,2.5};
const uint8_t ledMap[] = {0,1,2,3,4,5,6};
uint8_t mapping[][7] = {
{SK_S,SK_D,SK_F,SK_J,SK_K,SK_L,SK_SP},
{SK_ESC,SK_BKTK,SK_LEFT,SK_RIGHT,SK_UP,SK_DOWN,0},
{SK_F13,SK_F14,SK_F15,SK_F16,SK_F17,SK_F18,SK_F19},
{0,0,0,0,0,0,0},
{0,0,0,0,0,0,0},
{0,0,0,0,0,0,0},
{0,0,0,0,0,0,0}
};
#elif defined (G2x2)
const uint8_t ledMap[] = {0,1,3,2};
const float gridMap[] = {0,2,2,0};
uint8_t mapping[][4] = {
{SK_ESC,SK_BKTK,SK_Z,SK_X},
{SK_LEFT,SK_RIGHT,SK_UP,SK_DOWN},
{SK_F13,SK_F14,SK_F15,SK_F16},
{SK_MB1,SK_MB2,SK_MB4,SK_MB5}
};
#elif defined (MACRO)
const uint8_t ledMap[] = {0,1,2,5,4,3};
const float gridMap[] = {0,1,2,2,1,0};
uint8_t mapping[][6] = {
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F},
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F},
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F},
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F},
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F},
{SK_Z,SK_X,SK_C,SK_V,SK_D,SK_F}
};

// Set keys past 2 even if only 2 exist
#else
const uint8_t ledMap[] = {0,1,2,3};
const float gridMap[] = {0,1,2,3};
uint8_t mapping[][4] = {
{SK_Z,SK_X,SK_C,SK_V},
{SK_ESC,SK_BKTK,SK_LEFT,SK_RIGHT}
};
#endif
