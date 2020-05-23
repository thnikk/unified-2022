// Libraries
#include <Arduino.h>
#include <Adafruit_FreeTouch.h>
#include <FlashAsEEPROM.h>
#include <Bounce2.h>
//#include <Keyboard.h>
#include <HID-Project.h>
#include <FastLED.h>

// Initialize inputs and LEDs
Bounce * bounce = new Bounce[numkeys+1];
Adafruit_FreeTouch qt = Adafruit_FreeTouch(A0, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
CRGBArray<numkeys> leds;

const byte pins[] = { 12, 11 };
uint8_t mapping[][2] = {
{122,120},
{KEY_VOLUME_UP,KEY_VOLUME_DOWN}
};

bool pressed[numkeys+1];
bool lastPressed[numkeys+1];

byte b = 127;
byte bMax = 255;

byte ledMode = 2;
byte effectSpeed = 10;

// Colors for custom LED mode
// These are the initial values stored before changed through the remapper
byte custColor[] = { 224, 192 };

// BPS
byte bpsCount;

// FreeTouch
int touchValue;
int touchThreshold = 500;

// Millis timer for idle check
unsigned long pm;

const byte gridMap[] = {0, 1, 3, 2};


// Remapper
const String friendlyKeys[] = {
    "LEFT_CTRL", "LEFT_SHIFT", "LEFT_ALT", "LEFT_GUI", "RIGHT_CTRL", "RIGHT_SHIFT",
    "RIGHT_ALT", "RIGHT_GUI", "ESC", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
    "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
    "F21", "F22", "F23", "F24", "ENTER", "BACKSPACE", "TAB", "PRINT", "PAUSE", "INSERT",
    "HOME", "PAGE_UP", "DELETE", "END", "PAGE_DOWN", "RIGHT", "LEFT", "DOWN", "UP",
    "PAD_DIV", "PAD_MULT", "PAD_SUB", "PAD_ADD", "PAD_ENTER", "PAD_1", "PAD_2", "PAD_3",
    "PAD_4", "PAD_5", "PAD_6", "PAD_7", "PAD_8", "PAD_9", "PAD_0", "PAD_DOT", "MENU",
    "VOL_MUTE", "VOL_UP", "VOL_DOWN"
};
const uint8_t keycodes[] = {
    KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_LEFT_GUI, KEY_RIGHT_CTRL,
    KEY_RIGHT_SHIFT, KEY_RIGHT_ALT, KEY_RIGHT_GUI, KEY_ESC, KEY_F1, KEY_F2, KEY_F3,
    KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F13,
    KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22,
    KEY_F23, KEY_F24, KEY_ENTER, KEY_BACKSPACE, KEY_TAB, KEY_PRINT, KEY_PAUSE,
    KEY_INSERT, KEY_HOME, KEY_PAGE_UP, KEY_DELETE, KEY_END, KEY_PAGE_DOWN, KEY_RIGHT,
    KEY_LEFT, KEY_DOWN, KEY_UP, KEYPAD_DIVIDE, KEYPAD_MULTIPLY, KEYPAD_SUBTRACT,
    KEYPAD_ADD, KEYPAD_ENTER, KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_4, KEYPAD_5, KEYPAD_6,
    KEYPAD_7, KEYPAD_8, KEYPAD_9, KEYPAD_0, KEYPAD_DOT, KEY_MENU, KEY_VOLUME_MUTE,
    KEY_VOLUME_UP, KEY_VOLUME_DOWN
};

void eepromInit(){
    // If first boot after programming
    if (!EEPROM.isValid()) {
        // Assign default values
        EEPROM.write(1, bMax);
        b = bMax;
        EEPROM.write(2, ledMode);
        for (byte x=0; x<numkeys; x++) {
            EEPROM.write(3+x, custColor[x]);
        }
        // Mapping
        for (byte y=0; y<2; y++) {
            for (byte x=0; x<numkeys; x++) {
                int address=20+(y*30)+x;
                EEPROM.write(address, mapping[y][x]);
            }
        }
        // Write values
        EEPROM.commit();
    }
    // Otherwise, restore values
    else {
        bMax = EEPROM.read(1);
        ledMode = EEPROM.read(2);
        for (byte x=0;x<numkeys;x++){
            custColor[x] = EEPROM.read(3+x);
        }
        for (byte y=0; y<2; y++) {
            for (byte x=0; x<numkeys; x++) {
                int address=20+(y*30)+x;
                mapping[y][x] = EEPROM.read(address);
            }
        }
    }
}

void eepromUpdate(){
    if (bMax != EEPROM.read(1)) EEPROM.write(1, bMax);
    if (ledMode != EEPROM.read(2)) EEPROM.write(2, ledMode);
    for (byte x=0; x<numkeys; x++) {
        if (custColor[x] != EEPROM.read(3+x)) EEPROM.write(3+x, custColor[x]);
    }
    // Mapping
    for (byte y=0; y<2; y++) {
        for (byte x=0; x<numkeys; x++) {
            int address=20+(y*30)+x;
            if (mapping[y][x] != EEPROM.read(address)) EEPROM.write(address, mapping[y][x]);
        }
    }
    EEPROM.commit();
}

void setup() {
    // Set the serial baudrate
    SerialUSB.begin(9600);

    // Start LEDs
    FastLED.addLeds<NEOPIXEL, 13>(leds, numkeys);

    // Set brightness
    FastLED.setBrightness(255);

    // THIS SECTION NEEDS TO BE CHANGED FOR FULL FREETOUCH SUPPORT

    // Set pullups and attach pins to debounce lib with debounce time (in ms)
    for (byte x=0; x<=numkeys; x++) {
        pinMode(pins[x], INPUT_PULLUP);
        bounce[x].attach(pins[x]);
        bounce[x].interval(20);
    }

    // Initialize EEPROM
    eepromInit();

    // Start freetouch for side button
    qt.begin();
    NKROKeyboard.begin();
}

unsigned long checkMillis;
// All inputs are processed into pressed array
void checkState() {
    // Limiting input polling to polling rate increases speed by 17x!
    // This has no impact on efficacy while making it stupid fast.
    if ((millis() - checkMillis) >= 1) {
        // Write bounce values to main pressed array
        for(byte x=0; x<=numkeys; x++){ bounce[x].update(); pressed[x] = bounce[x].read(); }
        // Get current touch value and write to main array
        touchValue = qt.measure();
        // If it goes over the threshold value, the button is pressed
        if (touchValue > touchThreshold) pressed[numkeys] = 0;
        // To release, it must go under the threshold value by an extra 50 to avoid constantly changing
        else if ( touchValue < touchThreshold-50 ) pressed[numkeys] = 1;
#ifdef DEBUG
        SerialUSB.print("[");
        for (byte x=0;x<=numkeys;x++) {
            SerialUSB.print(pressed[x]);
            if (x != numkeys) SerialUSB.print(',');
        }
        SerialUSB.println("]");
#endif
        checkMillis=millis();
    }
}

// Compares inputs to last inputs and presses/releases key based on state change
void keyboard() {
    for (byte x=0; x<numkeys; x++){
        // If the button state changes, press/release a key.
        if ( pressed[x] != lastPressed[x] ){
            if (!pressed[x]) bpsCount++;
            pm = millis();
            uint8_t key = mapping[!pressed[numkeys]][x];
            uint8_t unKey = mapping[pressed[numkeys]][x];

            // Check press state and press/release key
            switch(key){
                // Key exceptions need to go here for NKROKeyboard
                case KEY_ESC: if (!pressed[x]) NKROKeyboard.press(KEY_ESC); if (pressed[x]) NKROKeyboard.release(KEY_ESC); break;
                case KEY_F1: if (!pressed[x]) NKROKeyboard.press(KEY_F1); if (pressed[x]) NKROKeyboard.release(KEY_F1); break;
                case KEY_F2: if (!pressed[x]) NKROKeyboard.press(KEY_F2); if (pressed[x]) NKROKeyboard.release(KEY_F2); break;
                case KEY_F3: if (!pressed[x]) NKROKeyboard.press(KEY_F3); if (pressed[x]) NKROKeyboard.release(KEY_F3); break;
                case KEY_F4: if (!pressed[x]) NKROKeyboard.press(KEY_F4); if (pressed[x]) NKROKeyboard.release(KEY_F4); break;
                case KEY_F5: if (!pressed[x]) NKROKeyboard.press(KEY_F5); if (pressed[x]) NKROKeyboard.release(KEY_F5); break;
                case KEY_F6: if (!pressed[x]) NKROKeyboard.press(KEY_F6); if (pressed[x]) NKROKeyboard.release(KEY_F6); break;
                case KEY_F7: if (!pressed[x]) NKROKeyboard.press(KEY_F7); if (pressed[x]) NKROKeyboard.release(KEY_F7); break;
                case KEY_F8: if (!pressed[x]) NKROKeyboard.press(KEY_F8); if (pressed[x]) NKROKeyboard.release(KEY_F8); break;
                case KEY_F9: if (!pressed[x]) NKROKeyboard.press(KEY_F9); if (pressed[x]) NKROKeyboard.release(KEY_F9); break;
                case KEY_F10: if (!pressed[x]) NKROKeyboard.press(KEY_F10); if (pressed[x]) NKROKeyboard.release(KEY_F10); break;
                case KEY_F11: if (!pressed[x]) NKROKeyboard.press(KEY_F11); if (pressed[x]) NKROKeyboard.release(KEY_F11); break;
                case KEY_F12: if (!pressed[x]) NKROKeyboard.press(KEY_F12); if (pressed[x]) NKROKeyboard.release(KEY_F12); break;
                case KEY_F13: if (!pressed[x]) NKROKeyboard.press(KEY_F13); if (pressed[x]) NKROKeyboard.release(KEY_F13); break;
                case KEY_F14: if (!pressed[x]) NKROKeyboard.press(KEY_F14); if (pressed[x]) NKROKeyboard.release(KEY_F14); break;
                case KEY_F15: if (!pressed[x]) NKROKeyboard.press(KEY_F15); if (pressed[x]) NKROKeyboard.release(KEY_F15); break;
                case KEY_F16: if (!pressed[x]) NKROKeyboard.press(KEY_F16); if (pressed[x]) NKROKeyboard.release(KEY_F16); break;
                case KEY_F17: if (!pressed[x]) NKROKeyboard.press(KEY_F17); if (pressed[x]) NKROKeyboard.release(KEY_F17); break;
                case KEY_F18: if (!pressed[x]) NKROKeyboard.press(KEY_F18); if (pressed[x]) NKROKeyboard.release(KEY_F18); break;
                case KEY_F19: if (!pressed[x]) NKROKeyboard.press(KEY_F19); if (pressed[x]) NKROKeyboard.release(KEY_F19); break;
                case KEY_F20: if (!pressed[x]) NKROKeyboard.press(KEY_F20); if (pressed[x]) NKROKeyboard.release(KEY_F20); break;
                case KEY_F21: if (!pressed[x]) NKROKeyboard.press(KEY_F21); if (pressed[x]) NKROKeyboard.release(KEY_F21); break;
                case KEY_F22: if (!pressed[x]) NKROKeyboard.press(KEY_F22); if (pressed[x]) NKROKeyboard.release(KEY_F22); break;
                case KEY_F23: if (!pressed[x]) NKROKeyboard.press(KEY_F23); if (pressed[x]) NKROKeyboard.release(KEY_F23); break;
                case KEY_F24: if (!pressed[x]) NKROKeyboard.press(KEY_F24); if (pressed[x]) NKROKeyboard.release(KEY_F24); break;
                case KEY_ENTER: if (!pressed[x]) NKROKeyboard.press(KEY_ENTER); if (pressed[x]) NKROKeyboard.release(KEY_ENTER); break;
                case KEY_BACKSPACE: if (!pressed[x]) NKROKeyboard.press(KEY_BACKSPACE); if (pressed[x]) NKROKeyboard.release(KEY_BACKSPACE); break;
                case KEY_TAB: if (!pressed[x]) NKROKeyboard.press(KEY_TAB); if (pressed[x]) NKROKeyboard.release(KEY_TAB); break;
                case KEY_PRINT: if (!pressed[x]) NKROKeyboard.press(KEY_PRINT); if (pressed[x]) NKROKeyboard.release(KEY_PRINT); break;
                case KEY_PAUSE: if (!pressed[x]) NKROKeyboard.press(KEY_PAUSE); if (pressed[x]) NKROKeyboard.release(KEY_PAUSE); break;
                case KEY_INSERT: if (!pressed[x]) NKROKeyboard.press(KEY_INSERT); if (pressed[x]) NKROKeyboard.release(KEY_INSERT); break;
                case KEY_HOME: if (!pressed[x]) NKROKeyboard.press(KEY_HOME); if (pressed[x]) NKROKeyboard.release(KEY_HOME); break;
                case KEY_PAGE_UP: if (!pressed[x]) NKROKeyboard.press(KEY_PAGE_UP); if (pressed[x]) NKROKeyboard.release(KEY_PAGE_UP); break;
                case KEY_DELETE: if (!pressed[x]) NKROKeyboard.press(KEY_DELETE); if (pressed[x]) NKROKeyboard.release(KEY_DELETE); break;
                case KEY_END: if (!pressed[x]) NKROKeyboard.press(KEY_END); if (pressed[x]) NKROKeyboard.release(KEY_END); break;
                case KEY_PAGE_DOWN: if (!pressed[x]) NKROKeyboard.press(KEY_PAGE_DOWN); if (pressed[x]) NKROKeyboard.release(KEY_PAGE_DOWN); break;
                case KEY_RIGHT: if (!pressed[x]) NKROKeyboard.press(KEY_RIGHT); if (pressed[x]) NKROKeyboard.release(KEY_RIGHT); break;
                case KEY_LEFT: if (!pressed[x]) NKROKeyboard.press(KEY_LEFT); if (pressed[x]) NKROKeyboard.release(KEY_LEFT); break;
                case KEY_DOWN: if (!pressed[x]) NKROKeyboard.press(KEY_DOWN); if (pressed[x]) NKROKeyboard.release(KEY_DOWN); break;
                case KEY_UP: if (!pressed[x]) NKROKeyboard.press(KEY_UP); if (pressed[x]) NKROKeyboard.release(KEY_UP); break;
                case KEYPAD_DIVIDE: if (!pressed[x]) NKROKeyboard.press(KEYPAD_DIVIDE); if (pressed[x]) NKROKeyboard.release(KEYPAD_DIVIDE); break;
                case KEYPAD_MULTIPLY: if (!pressed[x]) NKROKeyboard.press(KEYPAD_MULTIPLY); if (pressed[x]) NKROKeyboard.release(KEYPAD_MULTIPLY); break;
                case KEYPAD_SUBTRACT: if (!pressed[x]) NKROKeyboard.press(KEYPAD_SUBTRACT); if (pressed[x]) NKROKeyboard.release(KEYPAD_SUBTRACT); break;
                case KEYPAD_ADD: if (!pressed[x]) NKROKeyboard.press(KEYPAD_ADD); if (pressed[x]) NKROKeyboard.release(KEYPAD_ADD); break;
                case KEYPAD_ENTER: if (!pressed[x]) NKROKeyboard.press(KEYPAD_ENTER); if (pressed[x]) NKROKeyboard.release(KEYPAD_ENTER); break;
                case KEYPAD_1: if (!pressed[x]) NKROKeyboard.press(KEYPAD_1); if (pressed[x]) NKROKeyboard.release(KEYPAD_1); break;
                case KEYPAD_2: if (!pressed[x]) NKROKeyboard.press(KEYPAD_2); if (pressed[x]) NKROKeyboard.release(KEYPAD_2); break;
                case KEYPAD_3: if (!pressed[x]) NKROKeyboard.press(KEYPAD_3); if (pressed[x]) NKROKeyboard.release(KEYPAD_3); break;
                case KEYPAD_4: if (!pressed[x]) NKROKeyboard.press(KEYPAD_4); if (pressed[x]) NKROKeyboard.release(KEYPAD_4); break;
                case KEYPAD_5: if (!pressed[x]) NKROKeyboard.press(KEYPAD_5); if (pressed[x]) NKROKeyboard.release(KEYPAD_5); break;
                case KEYPAD_6: if (!pressed[x]) NKROKeyboard.press(KEYPAD_6); if (pressed[x]) NKROKeyboard.release(KEYPAD_6); break;
                case KEYPAD_7: if (!pressed[x]) NKROKeyboard.press(KEYPAD_7); if (pressed[x]) NKROKeyboard.release(KEYPAD_7); break;
                case KEYPAD_8: if (!pressed[x]) NKROKeyboard.press(KEYPAD_8); if (pressed[x]) NKROKeyboard.release(KEYPAD_8); break;
                case KEYPAD_9: if (!pressed[x]) NKROKeyboard.press(KEYPAD_9); if (pressed[x]) NKROKeyboard.release(KEYPAD_9); break;
                case KEYPAD_0: if (!pressed[x]) NKROKeyboard.press(KEYPAD_0); if (pressed[x]) NKROKeyboard.release(KEYPAD_0); break;
                case KEYPAD_DOT: if (!pressed[x]) NKROKeyboard.press(KEYPAD_DOT); if (pressed[x]) NKROKeyboard.release(KEYPAD_DOT); break;
                case KEY_MENU: if (!pressed[x]) NKROKeyboard.press(KEY_MENU); if (pressed[x]) NKROKeyboard.release(KEY_MENU); break;
                case KEY_VOLUME_MUTE: if (!pressed[x]) NKROKeyboard.press(KEY_VOLUME_MUTE); if (pressed[x]) NKROKeyboard.release(KEY_VOLUME_MUTE); break;
                case KEY_VOLUME_UP: if (!pressed[x]) NKROKeyboard.press(KEY_VOLUME_UP); if (pressed[x]) NKROKeyboard.release(KEY_VOLUME_UP); break;
                case KEY_VOLUME_DOWN: if (!pressed[x]) NKROKeyboard.press(KEY_VOLUME_DOWN); if (pressed[x]) NKROKeyboard.release(KEY_VOLUME_DOWN); break;
                case KEY_LEFT_CTRL: if (!pressed[x]) NKROKeyboard.press(KEY_LEFT_CTRL); if (pressed[x]) NKROKeyboard.release(KEY_LEFT_CTRL); break;
                case KEY_LEFT_SHIFT: if (!pressed[x]) NKROKeyboard.press(KEY_LEFT_SHIFT); if (pressed[x]) NKROKeyboard.release(KEY_LEFT_SHIFT); break;
                case KEY_LEFT_ALT: if (!pressed[x]) NKROKeyboard.press(KEY_LEFT_ALT); if (pressed[x]) NKROKeyboard.release(KEY_LEFT_ALT); break;
                case KEY_LEFT_GUI: if (!pressed[x]) NKROKeyboard.press(KEY_LEFT_GUI); if (pressed[x]) NKROKeyboard.release(KEY_LEFT_GUI); break;
                case KEY_RIGHT_CTRL: if (!pressed[x]) NKROKeyboard.press(KEY_RIGHT_CTRL); if (pressed[x]) NKROKeyboard.release(KEY_RIGHT_CTRL); break;
                case KEY_RIGHT_SHIFT: if (!pressed[x]) NKROKeyboard.press(KEY_RIGHT_SHIFT); if (pressed[x]) NKROKeyboard.release(KEY_RIGHT_SHIFT); break;
                case KEY_RIGHT_ALT: if (!pressed[x]) NKROKeyboard.press(KEY_RIGHT_ALT); if (pressed[x]) NKROKeyboard.release(KEY_RIGHT_ALT); break;
                case KEY_RIGHT_GUI: if (!pressed[x]) NKROKeyboard.press(KEY_RIGHT_GUI); if (pressed[x]) NKROKeyboard.release(KEY_RIGHT_GUI); break;
                default: if (!pressed[x]) NKROKeyboard.press(key); if (pressed[x]) NKROKeyboard.release(key); break;
            }
            // Same for unkey
            switch(unKey){
                case KEY_ESC: NKROKeyboard.release(KEY_ESC); break;
                case KEY_F1: NKROKeyboard.release(KEY_F1); break;
                case KEY_F2: NKROKeyboard.release(KEY_F2); break;
                case KEY_F3: NKROKeyboard.release(KEY_F3); break;
                case KEY_F4: NKROKeyboard.release(KEY_F4); break;
                case KEY_F5: NKROKeyboard.release(KEY_F5); break;
                case KEY_F6: NKROKeyboard.release(KEY_F6); break;
                case KEY_F7: NKROKeyboard.release(KEY_F7); break;
                case KEY_F8: NKROKeyboard.release(KEY_F8); break;
                case KEY_F9: NKROKeyboard.release(KEY_F9); break;
                case KEY_F10: NKROKeyboard.release(KEY_F10); break;
                case KEY_F11: NKROKeyboard.release(KEY_F11); break;
                case KEY_F12: NKROKeyboard.release(KEY_F12); break;
                case KEY_F13: NKROKeyboard.release(KEY_F13); break;
                case KEY_F14: NKROKeyboard.release(KEY_F14); break;
                case KEY_F15: NKROKeyboard.release(KEY_F15); break;
                case KEY_F16: NKROKeyboard.release(KEY_F16); break;
                case KEY_F17: NKROKeyboard.release(KEY_F17); break;
                case KEY_F18: NKROKeyboard.release(KEY_F18); break;
                case KEY_F19: NKROKeyboard.release(KEY_F19); break;
                case KEY_F20: NKROKeyboard.release(KEY_F20); break;
                case KEY_F21: NKROKeyboard.release(KEY_F21); break;
                case KEY_F22: NKROKeyboard.release(KEY_F22); break;
                case KEY_F23: NKROKeyboard.release(KEY_F23); break;
                case KEY_F24: NKROKeyboard.release(KEY_F24); break;
                case KEY_ENTER: NKROKeyboard.release(KEY_ENTER); break;
                case KEY_BACKSPACE: NKROKeyboard.release(KEY_BACKSPACE); break;
                case KEY_TAB: NKROKeyboard.release(KEY_TAB); break;
                case KEY_PRINT: NKROKeyboard.release(KEY_PRINT); break;
                case KEY_PAUSE: NKROKeyboard.release(KEY_PAUSE); break;
                case KEY_INSERT: NKROKeyboard.release(KEY_INSERT); break;
                case KEY_HOME: NKROKeyboard.release(KEY_HOME); break;
                case KEY_PAGE_UP: NKROKeyboard.release(KEY_PAGE_UP); break;
                case KEY_DELETE: NKROKeyboard.release(KEY_DELETE); break;
                case KEY_END: NKROKeyboard.release(KEY_END); break;
                case KEY_PAGE_DOWN: NKROKeyboard.release(KEY_PAGE_DOWN); break;
                case KEY_RIGHT: NKROKeyboard.release(KEY_RIGHT); break;
                case KEY_LEFT: NKROKeyboard.release(KEY_LEFT); break;
                case KEY_DOWN: NKROKeyboard.release(KEY_DOWN); break;
                case KEY_UP: NKROKeyboard.release(KEY_UP); break;
                case KEYPAD_DIVIDE: NKROKeyboard.release(KEYPAD_DIVIDE); break;
                case KEYPAD_MULTIPLY: NKROKeyboard.release(KEYPAD_MULTIPLY); break;
                case KEYPAD_SUBTRACT: NKROKeyboard.release(KEYPAD_SUBTRACT); break;
                case KEYPAD_ADD: NKROKeyboard.release(KEYPAD_ADD); break;
                case KEYPAD_ENTER: NKROKeyboard.release(KEYPAD_ENTER); break;
                case KEYPAD_1: NKROKeyboard.release(KEYPAD_1); break;
                case KEYPAD_2: NKROKeyboard.release(KEYPAD_2); break;
                case KEYPAD_3: NKROKeyboard.release(KEYPAD_3); break;
                case KEYPAD_4: NKROKeyboard.release(KEYPAD_4); break;
                case KEYPAD_5: NKROKeyboard.release(KEYPAD_5); break;
                case KEYPAD_6: NKROKeyboard.release(KEYPAD_6); break;
                case KEYPAD_7: NKROKeyboard.release(KEYPAD_7); break;
                case KEYPAD_8: NKROKeyboard.release(KEYPAD_8); break;
                case KEYPAD_9: NKROKeyboard.release(KEYPAD_9); break;
                case KEYPAD_0: NKROKeyboard.release(KEYPAD_0); break;
                case KEYPAD_DOT: NKROKeyboard.release(KEYPAD_DOT); break;
                case KEY_MENU: NKROKeyboard.release(KEY_MENU); break;
                case KEY_VOLUME_MUTE: NKROKeyboard.release(KEY_VOLUME_MUTE); break;
                case KEY_VOLUME_UP: NKROKeyboard.release(KEY_VOLUME_UP); break;
                case KEY_VOLUME_DOWN: NKROKeyboard.release(KEY_VOLUME_DOWN); break;
                case KEY_LEFT_CTRL: NKROKeyboard.release(KEY_LEFT_CTRL); break;
                case KEY_LEFT_SHIFT: NKROKeyboard.release(KEY_LEFT_SHIFT); break;
                case KEY_LEFT_ALT: NKROKeyboard.release(KEY_LEFT_ALT); break;
                case KEY_LEFT_GUI: NKROKeyboard.release(KEY_LEFT_GUI); break;
                case KEY_RIGHT_CTRL: NKROKeyboard.release(KEY_RIGHT_CTRL); break;
                case KEY_RIGHT_SHIFT: NKROKeyboard.release(KEY_RIGHT_SHIFT); break;
                case KEY_RIGHT_ALT: NKROKeyboard.release(KEY_RIGHT_ALT); break;
                case KEY_RIGHT_GUI: NKROKeyboard.release(KEY_RIGHT_GUI); break;
                default: NKROKeyboard.release(unKey); break;
            }
            // Save last pressed state to buffer
            lastPressed[x] = pressed[x];
        }
    }
}

// Cycle through rainbow
void wheel(){
    static uint8_t hue;
    for(int i = 0; i < numkeys; i++) {
        byte z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(hue+(i*50),255,255);
        else {
            leds[z] = 0xFFFFFF;
        }
    }
    hue--;
    FastLED.show();
}

// Fade from white to rainbow to off
void rbFade(){
    static int hue; // Though these are 0-255, they are saved as ints to allow overflow
    static int sat[numkeys];
    static int val[numkeys];
    for(int i = 0; i < numkeys; i++) {
        if (!pressed[i]) {
            if (sat[i] < 255) sat[i] = sat[i]+8;
            if (sat[i] > 255) sat[i] = 255; // Keep saturation within byte range
            if (sat[i] == 255 && val[i] > 0) val[i] = val[i]-8;
            if (val[i] < 0) val[i] = 0; // Same for val
        }
        else {
            sat[i]=0;
            val[i]=255;
        }
        byte z = gridMap[i]; // Allows custom LED mapping (for grids)
        leds[z] = CHSV(hue+(i*50),sat[i],val[i]);
    }
    hue-=8;
    if (hue < 0) hue = 255;
    FastLED.show();
}

// Custom colors
void custom(){
    for(int i = 0; i < numkeys; i++) {
        byte z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(custColor[z],255,255);
        else leds[z] = 0xFFFFFF;
    }
    FastLED.show();
}

unsigned long avgMillis;
byte bpsColor;
byte lastColor;
void bps(){
    // Update values once per second
    if ((millis() - avgMillis) > 1000) {
        lastColor = bpsColor;
        if (bpsCount > 25) bpsCount = 25; // Max count value
        if (bpsCount < 1) bpsCount = 1; // Prevents color jump
        bpsColor = (bpsCount*10);
        bpsCount = 0;
        avgMillis = millis();
    }

    if (lastColor > bpsColor) lastColor--;
    if (lastColor < bpsColor) lastColor++;

    for(int i = 0; i < numkeys; i++) {
        byte z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(lastColor+100,255,255);
        else leds[z] = 0xFFFFFF;
    }
    FastLED.show();

}

unsigned long effectMillis;
void effects(byte speed, byte MODE) {
    // All LED modes should go here for universal speed control
    if ((millis() - effectMillis) > speed){
        // Select LED mode
        switch(MODE){
            case 0:
                wheel(); break;
            case 1:
                rbFade(); break;
            case 2:
                custom(); break;
            case 3:
                bps(); break;
        }

        // Fade brightness on idle change
        if (b < bMax) b++;
        if (b > bMax) b--;

        // Set brightness and global effect speed
        FastLED.setBrightness(b);
        effectMillis = millis();
    }
}

unsigned long speedCheckMillis;
int count;
void speedCheck() {
    count++;
    if ((millis() - speedCheckMillis) > 1000){
        SerialUSB.println(count);
        count = 0;
        speedCheckMillis = millis();
    }
}

// Menu text
const String greet[]={
    "Press 0 to enter the configurator.",
    "(Keys on the keypad are disabled while the configurator is open.)"
};
const String menu[]={
    "Welcome to the configurator! Enter:",
    "0 to save and exit",
    "1 to remap keys",
    "2 to set the LED mode",
    "3 to set the brightness",
    "4 to set the custom colors"
};
const String LEDmodes[]={
    "Select an LED mode. Enter:",
    "0 for Cycle",
    "1 for Reactive",
    "2 for Custom",
    "3 for BPS"
};
const String custExp[]={
    "Please enter a color value for the respective key.",
    "Colors are expressed as a 0-255 value, where:",
    "red=0, orange=32, yellow=64, green=96",
    "aqua=128, blue=160, purple=192, and pink=224"
};
const String remapExp[]={
    "Please enter",
    ""
};

void printBlock(byte block) {
    switch(block){
        // Greeter message
        case 0:
            for (byte x=0;x<2;x++) SerialUSB.println(greet[x]);
            break;
        case 1:
            for (byte x=0;x<6;x++) SerialUSB.println(menu[x]);
            break;
        case 2:
            for (byte x=0;x<5;x++) SerialUSB.println(LEDmodes[x]);
            break;
        case 3:
            SerialUSB.println("Enter a brightness vaule between 0 and 255.");
            SerialUSB.print("Current value: ");
            SerialUSB.print(bMax);
            break;
        case 4:
            for (byte x=0;x<4;x++) SerialUSB.println(custExp[x]);
            SerialUSB.print("Current values: ");
            for (byte x=0;x<numkeys;x++) {
                SerialUSB.print(custColor[x]);
                if (x != numkeys-1) SerialUSB.print(", ");
            }
            break;
        case 5:
            for (byte y=0;y<2;y++) {
                SerialUSB.print("Layer ");
                SerialUSB.print(y+1);
                SerialUSB.print(": ");
                for (byte x=0;x<numkeys;x++) { SerialUSB.print(char(mapping[y][x])); if (x<numkeys-1) SerialUSB.print(", "); }
                SerialUSB.println();
            }
            break;
    }
    // Add extra line break
    SerialUSB.println();
}

const String modeNames[]={ "Cycle", "Reactive", "Custom", "BPS" };
void ledMenu() {
    printBlock(2);
    while(true){
        int incomingByte = SerialUSB.read();
        if (incomingByte > 0){
            if (incomingByte>=48&&incomingByte<=51) {
                ledMode = incomingByte-48;
                SerialUSB.print("Selected ");
                SerialUSB.println(modeNames[ledMode]);
                SerialUSB.println();
                return;
            }
            else SerialUSB.println("Please enter a valid value.");
        }
    }
}

String inString = "";    // string to hold input
bool start=0;
// Converts incoming string chars to a byte
int parseByte(){
    while (true) {
        int incomingByte = SerialUSB.read();
        if (incomingByte > 0) {
            start=1;
            // Parse input
            if (isDigit(incomingByte)) {
              // convert the incoming byte to a char and add it to the string:
              inString += (char)incomingByte;
            }
        }
        // Convert to byte after receiving block
        if (incomingByte <= 0 && start == 1) {
            int value = inString.toInt();
            inString = "";
            if (value >= 0 && value <= 255) { start=0; return value; }
            else { start = 0; SerialUSB.print(value); SerialUSB.println(" is invalid. Please enter a valid value."); }
        }
    }
}

void brightMenu(){
    printBlock(3);
    while(true){
        bMax = parseByte();
        SerialUSB.print("Entered value: ");
        SerialUSB.println(bMax);
        SerialUSB.println();
        return;
    }
}

void customMenu(){
    printBlock(4);
    while(true){
        for(byte x=0;x<numkeys;x++){
            SerialUSB.print("Color for key ");
            SerialUSB.print(x+1);
            SerialUSB.print(": ");
            byte color = parseByte();
            SerialUSB.println(color);
            custColor[x] = color;
            effects(10,2);
        }
        SerialUSB.println();
        // Give user a second to see new color.
        // Confirmation would make more sense here.
        delay(1000);
        return;
    }
}

void remapMenu(){

    // Main loop
    while(true){
    byte layer;
    SerialUSB.println("Which layer would you like to remap?");
    SerialUSB.println("0 to exit");
    SerialUSB.println("1 to remap layer 1");
    SerialUSB.println("2 to remap layer 2");
    printBlock(5);

    // Select layer
    bool layerCheck = 0;
    while(layerCheck == 0){
        int incomingByte = SerialUSB.read();
        if (incomingByte > 0){
            if (incomingByte>=48&&incomingByte<=50) {
                layer = incomingByte-48;
                if (layer == 0) layerCheck = 1; // Return before printing if layer is 0
                else {
                    SerialUSB.print("Layer ");
                    SerialUSB.print(layer);
                    SerialUSB.print(": ");
                    layerCheck = 1;
                }
            }
            else SerialUSB.println("Please enter a valid value.");
        }
    }

    // Exit if user inputs 0
    if (layer == 0) return;

    // Remap selected layer
    for(byte x=0;x<numkeys;x++){
        bool mapCheck = 0;
        while(mapCheck == 0){
            int incomingByte = SerialUSB.read();
            if (incomingByte>=32&&incomingByte<=126) {
                byte key = incomingByte;
                SerialUSB.print(char(key));
                // Subtract 1 because array is 0 indexed
                mapping[layer-1][x] = key;
                // Separate by comma if not last value
                if (x<numkeys-1) SerialUSB.print(", ");
                // Otherwise, create newline
                else SerialUSB.println();
                mapCheck = 1;
            }
            else if(incomingByte > 0) SerialUSB.println("Please enter a valid value.");
        }
    }
    SerialUSB.println();
    }
}

void mainmenu() {
    printBlock(1);
    while(true){
        int incomingByte = SerialUSB.read();
        effects(5, 0);
        if (isDigit(incomingByte)){
            switch(incomingByte-48){
                case(0):
                    SerialUSB.println("Settings saved! Exiting...");
                    eepromUpdate();
                    printBlock(0);
                    pm = millis(); // Reset idle counter post-config
                    return;
                    break;
                case(1):
                    remapMenu();
                    printBlock(1);
                    break;
                case(2):
                    ledMenu();
                    printBlock(1);
                    break;
                case(3):
                    brightMenu();
                    printBlock(1);
                    break;
                case(4):
                    customMenu();
                    printBlock(1);
                    break;
                default:
                    SerialUSB.println("Please enter a valid value.");
                    break;
            }
        }
    }
}

bool set;
unsigned long remapMillis;
void serialCheck() {
    // Check for serial connection every 1s
    if ((millis() - remapMillis) > 1000){
        // If the serial monitor is opened, greet the user.
        if (SerialUSB && set == 0) { printBlock(0); set=1; }
        // If closed, reset greet message.
        if (!SerialUSB) set = 0;

        // Check incoming character if exists
        if (SerialUSB.available() > 0) {
            byte incomingByte = SerialUSB.read();
            // If it's not 0, repeat the greet message
            if (incomingByte != 48) printBlock(0);
            // Otherwise, enter the menu.
            else mainmenu();
        }

        remapMillis = millis();
    }
}

void idle(){
    if ((millis() - pm) > 60000) bMax = 0;
    // Restore from EEPROM value here
    else bMax = EEPROM.read(1);
}

void loop() {
    // Update key state to check for button presses
    checkState();
    // Convert key presses to actual keyboard keys
    keyboard();
    // Make lights happen
    effects(10, ledMode);
    idle();
    // Debug check for loops per second
    speedCheck();
    serialCheck();
}
