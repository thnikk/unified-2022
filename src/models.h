// Helps keyboard code not wrap
#define KBP NKROKeyboard.press
#define KBR NKROKeyboard.release

// Platform specific
#ifdef TOUCH
#include <Adafruit_FreeTouch.h>
#endif

#ifdef TOUCH
#define neopin 3
Adafruit_FreeTouch qt_1 = Adafruit_FreeTouch(1, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_2 = Adafruit_FreeTouch(3, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
Adafruit_FreeTouch qt_3 = Adafruit_FreeTouch(4, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
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

#ifdef TOUCH
    const uint8_t pins[] = { A0, A1, A2, A3 };
    #if numkeys == 3
    uint8_t mapping[] = {SK_Z, SK_X, SK_ESC};
    #elif numkeys == 4
    uint8_t mapping[] = {SK_Z, SK_X, SK_ESC, SK_BKTK};
    #endif
#else
    #if numkeys == 3
    const uint8_t pins[] = { 2, 3, 1 };
    uint8_t mapping[] = {SK_Z, SK_X, SK_ESC};
    #elif numkeys == 5
    const uint8_t pins[] = { 2, 3, 8, 7, 1 };
    uint8_t mapping[] = {SK_Z, SK_X, SK_C, SK_V, SK_ESC};
    #endif
#endif
