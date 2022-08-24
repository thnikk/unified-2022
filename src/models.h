// Helps keyboard code not wrap
#define KBP NKROKeyboard.press
#define KBR NKROKeyboard.release

// Platform specific
#ifdef TOUCH
#include <Adafruit_FreeTouch.h>
#endif

// Mapping nicknames
// These are only used by the compiler so keys can have nicknames during assignment
#define SK_LEFT  173
#define SK_RIGHT 172
#define SK_UP    175
#define SK_DOWN  174
#define SK_Z     122
#define SK_X     120
#define SK_C     99
#define SK_V     118
#define SK_Q     113
#define SK_W     119
#define SK_E     101
#define SK_A     97
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

// This whole section is really ugly.
#ifdef TOUCH
    // For 2x2 prototype, this should be changed to the xiao pins
    #if defined(ALTPINS)
        const uint8_t pins[] = { 7, 6, 0, 1 };
        static uint8_t threshold[] = { 120, 120, 200, 150 };
        uint8_t mapping[] = {SK_Z, SK_X, SK_ESC, SK_BKTK};
        #warning Using alt pin mapping
    #elif defined(XIAO)
        // mini xiao
        #if numkeys == 2
            const uint8_t pins[] = { 0, 1 };
            static uint8_t threshold[] = { 220, 220 };
            uint8_t mapping[] = {SK_Z, SK_X };
        // mega xiao
        #elif numkeys == 4
            const uint8_t pins[] = { 0, 1, 7, 8 };
            static uint8_t threshold[] = { 220, 220, 220, 220 };
            uint8_t mapping[] = {SK_Z, SK_X, SK_ESC, SK_BKTK};
        // 4k mega xiao
        #elif numkeys == 6
            const uint8_t pins[] = { 0, 1, 6, 7, 8, 9 };
            static uint8_t threshold[] = { 220, 225, 190, 190, 190, 190 };
            uint8_t mapping[] = {SK_Z, SK_X, SK_C, SK_V, SK_ESC, SK_BKTK};
        #endif
    #else
        // mini
        #if numkeys == 2
            static uint8_t threshold[] = { 200, 175 };
            const uint8_t pins[] = { A0, A1 };
            uint8_t mapping[] = {SK_Z, SK_X};
        // mega
        #elif numkeys == 4
            static uint8_t threshold[] = { 175, 175, 150, 100 };
            const uint8_t pins[] = { A0, A1, A2, A3 };
            uint8_t mapping[] = {SK_Z, SK_X, SK_ESC, SK_BKTK};
        // 6k
        #elif numkeys == 6
            static uint8_t threshold[] = { 170, 165, 130, 120, 110, 135 };
            const uint8_t pins[] = { A0, A1, A2, A3, A6, A7 };
            uint8_t mapping[] = {SK_Q, SK_W, SK_E, SK_A, SK_S, SK_D};
        #endif
    #endif
#else
    // Empty array
    uint8_t threshold[numkeys];
    // 2k RGB
    #if numkeys == 3
    const uint8_t pins[] = { 2, 3, 1 };
    uint8_t mapping[] = {SK_Z, SK_X, SK_ESC};
    // 4K RGB
    #elif numkeys == 5
    const uint8_t pins[] = { 2, 3, 8, 7, 1 };
    uint8_t mapping[] = {SK_Z, SK_X, SK_C, SK_V, SK_ESC};
    // 7K RGB
    #elif numkeys == 9
    const uint8_t pins[] = { 1, 2, 3, 10, 9, 8, 6, 5, 7};
    uint8_t mapping[] = {SK_S, SK_D, SK_F, SK_J, SK_K, SK_L, SK_SP, SK_ESC, SK_BKTK};
    #endif
#endif

#ifdef TOUCH
Adafruit_FreeTouch qt[numkeys];
#endif
