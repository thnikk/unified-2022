// AVR specific
#ifdef AVR
#include <EEPROM.h>
const uint8_t pins[] = { 2, 3, 7, 9, 10, 11, 12, 4 };
#define NPPIN 5
#define COMMIT
#else
#include <FlashAsEEPROM.h>
#include <Adafruit_FreeTouch.h>
#define COMMIT EEPROM.commit();
#endif

// Pins for Trinket-based 2/4k keypads
#ifdef TRINKETM0
const uint8_t pins[] = { 0, 2, 20, 19, 3 };
#define NPPIN 1
#define TPIN 4
#endif

// Pins for Trinket-based 2k touch keypad
#ifdef TRINKETM0TOUCH
const uint_t pins[] = { 3, 4, 1 };
#endif

// Pins for all newer models
#ifdef SAMD21MINI
const uint8_t pins[] = { 12, 11, 10, 9, 6, 5, 0 };
#define NPPIN 13
#define TPIN A0
#define Serial SerialUSB
#endif
