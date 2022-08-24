// Libraries
#include <Arduino.h>
#include <Bounce2.h>
#include <HID-Project.h>
#include <Adafruit_NeoPixel.h>
// Pins, mappings, and board-specific libraries in this file
#include <models.h>
#include <FlashAsEEPROM.h>

#ifndef LED_TYPE
#define LED_TYPE NEO_GRB
#endif
Adafruit_NeoPixel pixels(numleds, neopin, LED_TYPE); 

Bounce * bounce = new Bounce[numkeys];

// buffer for keypresses
static bool pressed[numkeys];
static bool lastPressed[numkeys];

// Starting and max brightness
static uint8_t b = 127;
static uint8_t bMax = b;

// Default LED mode
static uint8_t ledMode = 0;

// Default debounce interval
static uint8_t debounceInterval = 4;

// Default reset value for touch pads
static uint8_t resetValue = 12;

// Colors for custom LED mode
// These are the initial values stored before changed through the remapper
static uint8_t custColor[] = {224,192,224,192,224,192,224};

// BPS
static uint8_t bpsCount;

// Default idle time
static byte idleMinutes = 5;

// Millis timer for idle check
static unsigned long pm;

// Display names for each key (in specific order, do not re-arrange)
const String friendlyKeys[] = {
    "L_CTRL", "L_SHIFT", "L_ALT", "L_GUI", "R_CTRL", "R_SHIFT",
    "R_ALT", "R_GUI", "ESC", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
    "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
    "F21", "F22", "F23", "F24", "ENTER", "BACKSP", "TAB", "PRINT", "PAUSE", "INSERT",
    "HOME", "PAGE_UP", "DELETE", "END", "PAGE_DN", "RIGHT", "LEFT", "DOWN", "UP",
    "PAD_DIV", "PAD_MULT", "PAD_SUB", "PAD_ADD", "PAD_ENT", "PAD_1", "PAD_2", "PAD_3",
    "PAD_4", "PAD_5", "PAD_6", "PAD_7", "PAD_8", "PAD_9", "PAD_0", "MENU",
    "V_MUTE", "V_UP", "V_DOWN", "M1", "M2", "M3", "M4", "M5"
};
const byte numSpecial = 71;

// Check if any key has been pressed in the loop.
static bool anyPressed = 0;

// Leave space for general settings before colors
const byte colAddr = 20;
// Start mapping after colors in EEPROM
const byte mapAddr = colAddr+numkeys;
const byte threshAddr = colAddr+numkeys+numkeys;

void eepromLoad(){
    bMax = EEPROM.read(1);
    ledMode = EEPROM.read(2);
    idleMinutes = EEPROM.read(3);
    debounceInterval = EEPROM.read(4);
    resetValue = EEPROM.read(5);
    for (uint8_t x=0;x<numkeys;x++) {
        custColor[x] = EEPROM.read(colAddr+x);
        mapping[x] = EEPROM.read(mapAddr+x);
        threshold[x] = EEPROM.read(threshAddr+x);
    }
}

void eepromUpdate(){
    // If values don't match, update them
    if (bMax != EEPROM.read(1)) EEPROM.write(1, bMax);
    if (ledMode != EEPROM.read(2)) EEPROM.write(2, ledMode);
    if (idleMinutes != EEPROM.read(3)) EEPROM.write(3, idleMinutes);
    if (debounceInterval != EEPROM.read(4)) EEPROM.write(4, debounceInterval);
    if (resetValue != EEPROM.read(5)) EEPROM.write(5, resetValue);
    for (uint8_t x=0; x<numkeys; x++) {
        if (custColor[x] != EEPROM.read(colAddr+x)) EEPROM.write(colAddr+x, custColor[x]);
        if (mapping[x] != EEPROM.read(mapAddr+x)) EEPROM.write(mapAddr+x, mapping[x]);
        if (threshold[x] != EEPROM.read(threshAddr+x)) EEPROM.write(threshAddr+x, threshold[x]);
    }
    EEPROM.commit();
}

void setup() {
    // Fix QTPY NeoPixel
    #ifdef QTPY
    PORT->Group[0].PINCFG[15].bit.DRVSTR = 1;
    #endif

    // Set the serial baudrate
    Serial.begin(9600);

    // Initialize LEDs
    pixels.begin();
    pixels.show();

    // Initialize EEPROM
    if (!EEPROM.isValid()) eepromUpdate();
    else eepromLoad();

// Initialize touchpads
#ifdef TOUCH
    for (uint8_t x=0; x<numkeys; x++) qt[x].begin();
    #ifdef XIAO
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    #else
    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);
    #endif
#else
    // Set pullups and attach pins to debounce lib with debounce time (in ms)
    for (uint8_t x=0; x<numkeys; x++) {
        pinMode(pins[x], INPUT_PULLUP);
        bounce[x].attach(pins[x]);
        bounce[x].interval(debounceInterval);
    }
    pinMode(11, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
#endif

    NKROKeyboard.begin();
    Mouse.begin();
}

unsigned long touchMillis;
uint8_t tv[numkeys];

void checkKeys() {
    anyPressed = 0;
#if defined (TOUCH)
    if ((millis() - touchMillis) > 0) {
        for (uint8_t x=0; x<numkeys; x++) tv[x] = qt[x].measure()/4;
        for (uint8_t x=0; x<numkeys; x++) {
            if (tv[x] > threshold[x]) pressed[x] = 0;
            else if ( tv[x] < threshold[x] - resetValue ) pressed[x] = 1;
        }
        touchMillis = millis();
    }
#else
// Regular models can just iterate with a for loop.
    for(uint8_t x=0; x<numkeys; x++){
        bounce[x].update();
        pressed[x] = bounce[x].read();
        if (!pressed[x]) anyPressed = 1;
    }
#endif
    // Always update anyPressed
    for(uint8_t x=0; x<numkeys; x++) if (!pressed[x]) anyPressed = 1;
}

// Check x key state
void serialCheck(uint8_t x) {
    Serial.print("Key ");
    Serial.print(x+1);
    Serial.print(" has been ");
    if (pressed[x] == 0) Serial.println("pressed.");
    if (pressed[x] == 1) Serial.println("released.");
}

void keyboard() {
    for (uint8_t x=0; x<numkeys; x++){
        // If the button state changes, press/release a key.
        if ( pressed[x] != lastPressed[x] ){
#ifdef DEBUG
            serialCheck(x); // Only prints on state change
#endif
            if (!pressed[x]) bpsCount++;
            pm = millis();
            // Check press state and press/release key
            switch(mapping[x]){
                // Key exceptions need to go here for NKROKeyboard
                // It would be really nice if there was a better way to do this,
                // but NKROKeyboard requires the literal key definition
                case 128: if (!pressed[x]) KBP(KEY_LEFT_CTRL); if (pressed[x]) KBR(KEY_LEFT_CTRL); break;
                case 129: if (!pressed[x]) KBP(KEY_LEFT_SHIFT); if (pressed[x]) KBR(KEY_LEFT_SHIFT); break;
                case 130: if (!pressed[x]) KBP(KEY_LEFT_ALT); if (pressed[x]) KBR(KEY_LEFT_ALT); break;
                case 131: if (!pressed[x]) KBP(KEY_LEFT_GUI); if (pressed[x]) KBR(KEY_LEFT_GUI); break;
                case 132: if (!pressed[x]) KBP(KEY_RIGHT_CTRL); if (pressed[x]) KBR(KEY_RIGHT_CTRL); break;
                case 133: if (!pressed[x]) KBP(KEY_RIGHT_SHIFT); if (pressed[x]) KBR(KEY_RIGHT_SHIFT); break;
                case 134: if (!pressed[x]) KBP(KEY_RIGHT_ALT); if (pressed[x]) KBR(KEY_RIGHT_ALT); break;
                case 135: if (!pressed[x]) KBP(KEY_RIGHT_GUI); if (pressed[x]) KBR(KEY_RIGHT_GUI); break;
                case 136: if (!pressed[x]) KBP(KEY_ESC); if (pressed[x]) KBR(KEY_ESC); break;
                case 137: if (!pressed[x]) KBP(KEY_F1); if (pressed[x]) KBR(KEY_F1); break;
                case 138: if (!pressed[x]) KBP(KEY_F2); if (pressed[x]) KBR(KEY_F2); break;
                case 139: if (!pressed[x]) KBP(KEY_F3); if (pressed[x]) KBR(KEY_F3); break;
                case 140: if (!pressed[x]) KBP(KEY_F4); if (pressed[x]) KBR(KEY_F4); break;
                case 141: if (!pressed[x]) KBP(KEY_F5); if (pressed[x]) KBR(KEY_F5); break;
                case 142: if (!pressed[x]) KBP(KEY_F6); if (pressed[x]) KBR(KEY_F6); break;
                case 143: if (!pressed[x]) KBP(KEY_F7); if (pressed[x]) KBR(KEY_F7); break;
                case 144: if (!pressed[x]) KBP(KEY_F8); if (pressed[x]) KBR(KEY_F8); break;
                case 145: if (!pressed[x]) KBP(KEY_F9); if (pressed[x]) KBR(KEY_F9); break;
                case 146: if (!pressed[x]) KBP(KEY_F10); if (pressed[x]) KBR(KEY_F10); break;
                case 147: if (!pressed[x]) KBP(KEY_F11); if (pressed[x]) KBR(KEY_F11); break;
                case 148: if (!pressed[x]) KBP(KEY_F12); if (pressed[x]) KBR(KEY_F12); break;
                case 149: if (!pressed[x]) KBP(KEY_F13); if (pressed[x]) KBR(KEY_F13); break;
                case 150: if (!pressed[x]) KBP(KEY_F14); if (pressed[x]) KBR(KEY_F14); break;
                case 151: if (!pressed[x]) KBP(KEY_F15); if (pressed[x]) KBR(KEY_F15); break;
                case 152: if (!pressed[x]) KBP(KEY_F16); if (pressed[x]) KBR(KEY_F16); break;
                case 153: if (!pressed[x]) KBP(KEY_F17); if (pressed[x]) KBR(KEY_F17); break;
                case 154: if (!pressed[x]) KBP(KEY_F18); if (pressed[x]) KBR(KEY_F18); break;
                case 155: if (!pressed[x]) KBP(KEY_F19); if (pressed[x]) KBR(KEY_F19); break;
                case 156: if (!pressed[x]) KBP(KEY_F20); if (pressed[x]) KBR(KEY_F20); break;
                case 157: if (!pressed[x]) KBP(KEY_F21); if (pressed[x]) KBR(KEY_F21); break;
                case 158: if (!pressed[x]) KBP(KEY_F22); if (pressed[x]) KBR(KEY_F22); break;
                case 159: if (!pressed[x]) KBP(KEY_F23); if (pressed[x]) KBR(KEY_F23); break;
                case 160: if (!pressed[x]) KBP(KEY_F24); if (pressed[x]) KBR(KEY_F24); break;
                case 161: if (!pressed[x]) KBP(KEY_ENTER); if (pressed[x]) KBR(KEY_ENTER); break;
                case 162: if (!pressed[x]) KBP(KEY_BACKSPACE); if (pressed[x]) KBR(KEY_BACKSPACE); break;
                case 163: if (!pressed[x]) KBP(KEY_TAB); if (pressed[x]) KBR(KEY_TAB); break;
                case 164: if (!pressed[x]) KBP(KEY_PRINT); if (pressed[x]) KBR(KEY_PRINT); break;
                case 165: if (!pressed[x]) KBP(KEY_PAUSE); if (pressed[x]) KBR(KEY_PAUSE); break;
                case 166: if (!pressed[x]) KBP(KEY_INSERT); if (pressed[x]) KBR(KEY_INSERT); break;
                case 167: if (!pressed[x]) KBP(KEY_HOME); if (pressed[x]) KBR(KEY_HOME); break;
                case 168: if (!pressed[x]) KBP(KEY_PAGE_UP); if (pressed[x]) KBR(KEY_PAGE_UP); break;
                case 169: if (!pressed[x]) KBP(KEY_DELETE); if (pressed[x]) KBR(KEY_DELETE); break;
                case 170: if (!pressed[x]) KBP(KEY_END); if (pressed[x]) KBR(KEY_END); break;
                case 171: if (!pressed[x]) KBP(KEY_PAGE_DOWN); if (pressed[x]) KBR(KEY_PAGE_DOWN); break;
                case 172: if (!pressed[x]) KBP(KEY_RIGHT); if (pressed[x]) KBR(KEY_RIGHT); break;
                case 173: if (!pressed[x]) KBP(KEY_LEFT); if (pressed[x]) KBR(KEY_LEFT); break;
                case 174: if (!pressed[x]) KBP(KEY_DOWN); if (pressed[x]) KBR(KEY_DOWN); break;
                case 175: if (!pressed[x]) KBP(KEY_UP); if (pressed[x]) KBR(KEY_UP); break;
                case 176: if (!pressed[x]) KBP(KEYPAD_DIVIDE); if (pressed[x]) KBR(KEYPAD_DIVIDE); break;
                case 177: if (!pressed[x]) KBP(KEYPAD_MULTIPLY); if (pressed[x]) KBR(KEYPAD_MULTIPLY); break;
                case 178: if (!pressed[x]) KBP(KEYPAD_SUBTRACT); if (pressed[x]) KBR(KEYPAD_SUBTRACT); break;
                case 179: if (!pressed[x]) KBP(KEYPAD_ADD); if (pressed[x]) KBR(KEYPAD_ADD); break;
                case 180: if (!pressed[x]) KBP(KEYPAD_ENTER); if (pressed[x]) KBR(KEYPAD_ENTER); break;
                case 181: if (!pressed[x]) KBP(KEYPAD_1); if (pressed[x]) KBR(KEYPAD_1); break;
                case 182: if (!pressed[x]) KBP(KEYPAD_2); if (pressed[x]) KBR(KEYPAD_2); break;
                case 183: if (!pressed[x]) KBP(KEYPAD_3); if (pressed[x]) KBR(KEYPAD_3); break;
                case 184: if (!pressed[x]) KBP(KEYPAD_4); if (pressed[x]) KBR(KEYPAD_4); break;
                case 185: if (!pressed[x]) KBP(KEYPAD_5); if (pressed[x]) KBR(KEYPAD_5); break;
                case 186: if (!pressed[x]) KBP(KEYPAD_6); if (pressed[x]) KBR(KEYPAD_6); break;
                case 187: if (!pressed[x]) KBP(KEYPAD_7); if (pressed[x]) KBR(KEYPAD_7); break;
                case 188: if (!pressed[x]) KBP(KEYPAD_8); if (pressed[x]) KBR(KEYPAD_8); break;
                case 189: if (!pressed[x]) KBP(KEYPAD_9); if (pressed[x]) KBR(KEYPAD_9); break;
                case 190: if (!pressed[x]) KBP(KEYPAD_0); if (pressed[x]) KBR(KEYPAD_0); break;
                case 191: if (!pressed[x]) KBP(KEY_MENU); if (pressed[x]) KBR(KEY_MENU); break;
                case 192: if (!pressed[x]) KBP(KEY_VOLUME_MUTE); if (pressed[x]) KBR(KEY_VOLUME_MUTE); break;
                case 193: if (!pressed[x]) KBP(KEY_VOLUME_UP); if (pressed[x]) KBR(KEY_VOLUME_UP); break;
                case 194: if (!pressed[x]) KBP(KEY_VOLUME_DOWN); if (pressed[x]) KBR(KEY_VOLUME_DOWN); break;
                case 195: if (!pressed[x]) Mouse.press(MOUSE_LEFT); if (pressed[x]) Mouse.release(MOUSE_LEFT); break;
                case 196: if (!pressed[x]) Mouse.press(MOUSE_RIGHT); if (pressed[x]) Mouse.release(MOUSE_RIGHT); break;
                case 197: if (!pressed[x]) Mouse.press(MOUSE_MIDDLE); if (pressed[x]) Mouse.release(MOUSE_MIDDLE); break;
                case 198: if (!pressed[x]) Mouse.press(MOUSE_PREV); if (pressed[x]) Mouse.release(MOUSE_PREV); break;
                case 199: if (!pressed[x]) Mouse.press(MOUSE_NEXT); if (pressed[x]) Mouse.release(MOUSE_NEXT); break;
                default: if (!pressed[x]) KBP(mapping[x]); if (pressed[x]) KBR(mapping[x]); break;
            }
            // Save last pressed state to buffer
            lastPressed[x] = pressed[x];
        }
    }
}

uint32_t hsv_mult = 256;

// Cycle through rainbow
void wheel(){
    static uint8_t hue;
#if numleds == 1
    if (anyPressed == 0) pixels.setPixelColor(0, pixels.ColorHSV(hue*hsv_mult, 255, b));
    else pixels.setPixelColor(0, pixels.Color(255, 255, b));
#else
    for(uint8_t i = 0; i < numleds; i++) {
        if (pressed[i]) pixels.setPixelColor(i, pixels.ColorHSV((hue+(i*20))*hsv_mult, 255, b));
        else pixels.setPixelColor(i, pixels.Color(255, 255, b));
    }
#endif
    hue--;
    pixels.show();
}

// Highlight the key being remapped.
static uint8_t selected;
void highlightSelected(){
    uint8_t hue = (255/numkeys);
    for(uint8_t i = 0; i < numkeys; i++) {
        pixels.setPixelColor(i, pixels.ColorHSV(hue*hsv_mult, 255, b));
        if (i == selected) pixels.setPixelColor(i, pixels.Color(255, 255, b)); 
    }
#if numleds == 1
    if (anyPressed == 0) pixels.setPixelColor(0, pixels.ColorHSV(hue*hsv_mult, 255, b));
#endif
    pixels.show();
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
        //leds[i] = CHSV(hue+(i*50),sat[i],val[i]);
        pixels.setPixelColor(i, pixels.ColorHSV((hue+(i*50))*hsv_mult, sat[i], val[i] * b / 255));
    }
#if numleds == 1
    static int satDS;
    static int valDS;
    if (anyPressed) {
        if (satDS < 255) satDS = satDS+8;
        if (satDS > 255) satDS = 255; // Keep saturation within byte range
        if (satDS == 255 && valDS > 0) valDS = valDS-8;
        if (valDS < 0) valDS = 0; // Same for val
    }
    else { satDS=0; valDS=255; }
    //leds[0] = CHSV(hue,satDS,valDS);
    pixels.setPixelColor(0, pixels.ColorHSV(hue*hsv_mult, satDS, valDS * b / 255));
#endif
    hue-=8;
    if (hue < 0) hue = 255;
    pixels.show();
}

// Custom colors
void custom(){
    // Iterate through keys
    for(int i = 0; i < numkeys; i++) {
        // adjust LED order for special keypads
        //if (pressed[i]) leds[i] = CHSV(custColor[i],255,255);
        //else leds[i] = 0xFFFFFF;
        if (pressed[i]) pixels.setPixelColor(i, pixels.ColorHSV(custColor[i]*hsv_mult, 255, b));
        else pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
#if numleds == 1
    if (anyPressed == 0) pixels.setPixelColor(0, pixels.ColorHSV(custColor[0]*hsv_mult, 255, b));
    else pixels.setPixelColor(0, pixels.Color(255, 255, 255));
#endif
    //FastLED.show();
    pixels.show();
}

static unsigned long avgMillis;
static uint16_t bpsColor;
static uint16_t lastColor;
int incValue;
void bps(){
    // Update values once per second
    if ((millis() - avgMillis) > 1000) {
        lastColor = bpsColor;
        bpsColor = (bpsCount*10);
        bpsCount = 0;
        avgMillis = millis();
    }

    uint8_t bpsSpeed = 3;
    // Inc/dec values to smooth transition
    if (lastColor > bpsColor) {
        lastColor-=bpsSpeed;
        if (lastColor - bpsColor < bpsSpeed) lastColor=bpsColor;
    }
    if (lastColor < bpsColor){
        lastColor+=bpsSpeed;
        if (bpsColor - lastColor < bpsSpeed) lastColor=bpsColor;
    }

    uint8_t finalColor = lastColor%256;

    for(int i = 0; i < numleds; i++) {
        if (pressed[i]) pixels.setPixelColor(i, pixels.ColorHSV((finalColor+100)*hsv_mult, 255, b));
        else pixels.setPixelColor(i, pixels.Color(255, 255, b));
    }
#if numleds == 1
    if (anyPressed == 0) pixels.setPixelColor(0, pixels.ColorHSV((finalColor+100)*hsv_mult, 255, b));
    else pixels.setPixelColor(0, pixels.Color(255, 255, b));
#endif
    //FastLED.show();
    pixels.show();

}

static unsigned long effectMillis;
void effects(uint8_t speed, uint8_t MODE) {
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
            case 4:
                //highlightSelected();
                break;
        }

        // Fade brightness on idle change
        if (b < bMax) b++;
        if (b > bMax) b--;

        effectMillis = millis();
    }
}

static unsigned long serialDebugMillis;
static int count;
void serialDebug() {
    count++;
    if ((millis() - serialDebugMillis) > 1000){
        // Print brightness and EEPROM brightness
        Serial.print("Brightness: "); Serial.print(b); Serial.print(" / "); Serial.println(EEPROM.read(1));
        // Print EEPROM LED mode
        Serial.print("LED mode: "); Serial.println(EEPROM.read(2));
        // Print EEPROM idle timeout
        Serial.print("Idle timeout: "); Serial.println(EEPROM.read(3));
        // Print loops per second
        Serial.print("LPS: ");Serial.println(count);
        // Print seconds since last keypress (idle debugging)
        Serial.print("Seconds since last keypress: ");Serial.println((millis() - pm)/1000);
        // Print idle minutes var
        Serial.print("Idle minutes: ");Serial.println(idleMinutes);

        // Print current threshold values
        Serial.print("Touch sensitivity: ");
        for (uint8_t x=0; x<numkeys; x++) {
            Serial.print(threshold[x]);
            if (x<numkeys-1) Serial.print(", ");
            else Serial.println();
        }

        // Print touch values
        Serial.print("Touch values: ");
        for (uint8_t x=0; x<numkeys; x++) {
            Serial.print(tv[x]);
            if (x<numkeys-1) Serial.print(", ");
            else Serial.println();
        }

        count = 0;
        serialDebugMillis = millis();
    }
}

// Menu text
void greet(){
    Serial.println(F("Enter 'c' to start the configurator."));
    Serial.println(F("(Keys on the keypad are disabled while the configurator is open.)"));
}
void menu(){
    Serial.println(F("Welcome to the configurator! Enter:"));
    Serial.println(F("0 to save and exit"));
    Serial.println(F("1 to remap keys"));
    Serial.println(F("2 to set the LED mode"));
    Serial.println(F("3 to set the brightness"));
    Serial.println(F("4 to set the custom colors"));
    Serial.println(F("5 to set the idle timeout"));
#ifdef TOUCH
    Serial.println(F("6 to set the touch sensitivity"));
    Serial.println(F("7 to auto-calibrate touch sensitivity"));
    Serial.println(F("8 to set the touchpad reset value"));
#else
    Serial.println(F("6 to set the debounce interval"));
#endif
}
void LEDmodes(){
    Serial.println(F("Select an LED mode. Enter:"));
    Serial.println(F("0 for Cycle"));
    Serial.println(F("1 for Reactive"));
    Serial.println(F("2 for Custom"));
    Serial.println(F("3 for BPS"));
}
void custExp(){
    Serial.println(F("Please enter a color value for the respective key."));
    Serial.println(F("Colors are expressed as a 0-255 value, where:"));
    Serial.println(F("red=0, orange=32, yellow=64, green=96"));
    Serial.println(F("aqua=128, blue=160, purple=192, and pink=224"));
    Serial.print(F("Current values: "));
    for (uint8_t x=0;x<numleds;x++) {
        Serial.print(custColor[x]);
        if (x != numleds-1) Serial.print(", ");
    }
}
void remapExp(){
    Serial.println(F("If you're trying to map a key that doesn't print a character,"));
    Serial.println(F("please use one of the codes below with a ':' in front of it."));
}
void brightExp(){
    Serial.println(F("Enter a brightness value between 0 and 255."));
    Serial.print(F("Current value: "));
    Serial.print(bMax);
}
void idleExp(){
    Serial.println(F("Please enter an idle timeout value in minutes between 0 and 255."));
    Serial.println(F("A value of 0 will disable the idle timeout feature."));
    Serial.print(F("Current value: "));
    Serial.println(idleMinutes);
}
void resetExp(){
    Serial.println(F("Please enter a touchpad reset value between 0 and 255."));
    Serial.println(F("This value determines how much force is required for the release of a pad."));
    Serial.println(F("A sane value is 5-15. Below 5 is not recommended as it may cause"));
    Serial.println(F("the pad to spam inputs."));
    Serial.print(F("Current value: "));
    Serial.println(resetValue);
}
void debounceExp(){
    Serial.println(F("Enter a debounce value between 0 and 255."));
    Serial.println(F("A sane value is 2-10."));
    Serial.print(F("Current value: "));
    Serial.print(debounceInterval);
}
void thresholdExp(){
    Serial.println(F("Enter a sensitivity value for each pad between 0 and 255 (higher is less sensitive.)"));
    Serial.println(F("A sane value is 150-225."));
    Serial.print(F("Current values: "));
    for (uint8_t x=0; x<numkeys; x++) {
        Serial.print(threshold[x]);
        if (x<numkeys-1) Serial.print(", ");
        else Serial.println();
    }
}

void keyTable() {
    // Print welcome message
    remapExp();
    uint8_t lineLength = 0;
    // Print top line of table
    for (int y = 0; y < 69; y++) Serial.print("-");
    Serial.println();
    for (int x = 0; x <= numSpecial; x++) {
        if (lineLength == 0) Serial.print("| ");
        // Make every line wrap at 30 characters
        uint8_t nameLength = friendlyKeys[x].length(); // save as variable within for loop for repeated use
        lineLength += nameLength + 6;
        Serial.print(friendlyKeys[x]);
        nameLength = 9 - nameLength;
        while (nameLength > 0) { // Print a space
            Serial.print(" ");
            lineLength++;
            nameLength--;
        }
        if (x > 9) lineLength++;
        Serial.print(" = ");
        if (x <= 9) {
            Serial.print(" ");
            lineLength+=2;
        }
        Serial.print(x);
        Serial.print(" | ");
        if (lineLength > 55) {
            lineLength = 0;
            Serial.println();
        }
    }
    // If line isn't finished, create newline
    if (lineLength != 0) Serial.println();
    // Print bottom line of table
    for (int y = 0; y < 69; y++) Serial.print("-");
    Serial.println();
    Serial.println(F("For example, enter :8 to map escape"));
    Serial.println();
}

void keyLookup(uint8_t inByte) {
    for (uint8_t x=0; x<=numSpecial;x++) {
        if (inByte == 128+x) { Serial.print(friendlyKeys[x]); return; }
    }
    Serial.print(char(inByte));
}

// This funciton is redundant
void printBlock(uint8_t block) {
    switch(block){
        // Greeter message
        case 0: // Greeter
            greet();
            break;
        case 1: // Main menu
            menu();
            break;
        case 2: // LED Mode
            LEDmodes();
            break;
        case 3: // Brightness
            brightExp();
            break;
        case 4: // Custom colors
            custExp();
            break;
        case 5: // Remapper
            // Print key names and numbers
            Serial.println();
            Serial.println(F("Current values: "));
            for (uint8_t x=0;x<numkeys;x++) { keyLookup(mapping[x]); if (x<numkeys-1) Serial.print(", "); }
            Serial.println();
            break;
        case 6: // Brightness
            debounceExp();
            break;
        case 7: // Brightness
            thresholdExp();
            break;
    }
    // Add extra line break
    Serial.println();
}

const String modeNames[]={ "Cycle", "Reactive", "Custom", "BPS" };
void ledMenu() {
    printBlock(2);
    while(true){
        int incomingByte = Serial.read();
        if (incomingByte > 0){
            if (incomingByte>=48&&incomingByte<=51) {
                ledMode = incomingByte-48;
                Serial.print(F("Selected "));
                Serial.println(modeNames[ledMode]);
                Serial.println();
                return;
            }
            else Serial.println(F("Please enter a valid value."));
        }
    }
}

// Converts incoming string chars to a byte
int parseByte(){
    String inString = "";    // string to hold input
    bool start=0;
    while (true) {
        int incomingByte = Serial.read();
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
            else { start = 0; Serial.print(value); Serial.println(F(" is invalid. Please enter a valid value.")); }
        }
    }
}

// Converts incoming string chars to a byte
bool enterMenu(){
    String inString = "";    // string to hold input
    char inChar = Serial.read();
    // Wait for characters to stop coming in
    while (Serial.read() > 0) {
        inString += inChar;
    }
    // Then process
    if (inString == "menu") {
        inString = "";
        return 1;
    }
    else {
        inString = "";
        return 0;
    }
}

uint8_t parseKey() {
    String inString = "";    // string to hold input
    bool start=0;
    while (true) {
        // Highlight active key
        effects(5, 4);
        // Store incoming byte from Serial connection
        int inByte = Serial.read();
        // If the first character is a colon
        if (inByte == 58 && start == 0) {
            start = 1;
            // Only append it to the string if there's nothing after it
        }
        // If the first byte isn't a colon, just pass it through.
        else if (inByte > 0 && start == 0) {
            return inByte;
        }
        // If the first byte was a colon, start appending.
        else if (inByte > 0 && start == 1) {
            // Append if number, otherwise complain and start over.
            if (isDigit(inByte)) {
              // convert the incoming byte to a char and add it to the string:
              inString += (char)inByte;
            }
            else { inString=""; start=0; Serial.println(F("Please enter a valid value.")); }
        }
        // If block is finished being sent
        if (inByte <= 0 && start == 1) {
            // Convert string to int
            int value = inString.toInt();
            if (value >= 0 && value <= numSpecial) return 128+value;
            // Otherwise, restart
            else { inString=""; start = 0; Serial.print(value); Serial.println(F(" is invalid. Please enter a valid value.")); }
        }
    }
}

// Menu for changing brightness
uint8_t brightMenu(){
    while(true){
        uint8_t temp = parseByte();
        Serial.print(F("Entered value: "));
        Serial.println(temp);
        Serial.println();
        return temp;
    }
}

// Menu for changing colors on custom mode
void customMenu(){
    printBlock(4);
    while(true){
        for(uint8_t x=0;x<numleds;x++){
            Serial.print(F("Color for key "));
            Serial.print(x+1);
            Serial.print(": ");
            uint8_t color = parseByte();
            Serial.println(color);
            custColor[x] = color;
            effects(10,2);
        }
        Serial.println();
        // Give user a second to see new color.
        // Confirmation would make more sense here.
        delay(1000);
        return;
    }
}


// Menu for changing colors on custom mode
void thresholdMenu(){
    printBlock(7);
    while(true){
        for(uint8_t x=0;x<numkeys;x++){
            Serial.print(F("Sensitivity for key "));
            Serial.print(x+1);
            Serial.print(": ");
            uint8_t new_thresh = parseByte();
            Serial.println(new_thresh);
            threshold[x] = new_thresh;
        }
        Serial.println();
        return;
    }
}

// Menu for remapping
void remapMenu(){

    printBlock(5);

    keyTable();
    for(uint8_t x=0;x<numkeys;x++){
        selected = x;
        uint8_t key = parseKey();
        keyLookup(key);
        // Subtract 1 because array is 0 indexed
        mapping[x] = key;
        // Separate by comma if not last value
        if (x<numkeys) Serial.print(F(", "));
        // Otherwise, create newline
        else Serial.println();
    }
    Serial.println();
}

#ifdef TOUCH
void touch_calibrate() {
    Serial.println("Your touch pads will now be auto-calibrated.");
    Serial.println("Please press each pad in order with as much force");
    Serial.println("as you would like for them to be actuated with,");
    Serial.println("and wait for confirmation between pads.");
    static uint8_t mv[numkeys];
    for (uint8_t x=0; x<numkeys; x++) {
        static uint8_t init_value = tv[x];
        // If the touch value hasn't increased by 10
        while (tv[x] - init_value <= 5) {
            checkKeys(); // Update values or we'll get stuck forever
            delay(1); // Speed limit with delay, only check once per ms
        }
        // If the touch value has increased by 9
        while (tv[x] - init_value > 4) {
            checkKeys(); // Update values
            if (tv[x] > mv[x]) mv[x] = tv[x]; // Update max value if touch value is higher
            delay(1); // Speed limit with delay, only check once per ms
        }
        Serial.print("Value for pad ");
        Serial.print(x+1);
        Serial.print(": ");
        threshold[x] = ((mv[x] - init_value)/2) + init_value;
        Serial.println(threshold[x]);
    }
    Serial.println();
}
#endif

// Main menu
void mainmenu() {
    printBlock(1);
    while(true){
        int incomingByte = Serial.read();
        // While wating for a selection, cycle LEDs quickly.
        effects(5, 0);
        if (isDigit(incomingByte)){
            // Wait for a value to match
            switch(incomingByte-48){
                case(0):
                    Serial.println(F("Settings saved! Exiting..."));
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
                    printBlock(3);
                    bMax = brightMenu();
                    printBlock(1);
                    break;
                case(4):
                    customMenu();
                    printBlock(1);
                    break;
                case(5):
                    idleExp();
                    idleMinutes = brightMenu();
                    printBlock(1);
                    break;
#ifdef TOUCH
                case(6):
                    thresholdMenu();
                    printBlock(1);
                    break;
                case(7):
                    touch_calibrate();
                    printBlock(1);
                    break;
                case(8):
                    resetExp();
                    resetValue = brightMenu();
                    printBlock(1);
                    break;
#else
                case(6):
                    debounceExp();
                    debounceInterval = brightMenu();
                    for (uint8_t x=0; x<numkeys; x++) bounce[x].interval(debounceInterval);
                    printBlock(1);
                    break;
#endif
                default:
                    Serial.println(F("Please enter a valid value."));
                    break;
            }
        }
    }
}

static unsigned long remapMillis;
static unsigned long enterMillis;
void serialCheck() {
    // Check for serial connection every 1s
    if ((millis() - remapMillis) > 1000){
        // Push greeting
        if ((millis() - enterMillis) > 5000){
            printBlock(0);
            enterMillis = millis();
        }

        // Check incoming character if exists
        if (Serial.available() > 0) {
            char inChar = Serial.read();
            // If special key is received, enter the configurator
            if (inChar == 'c') mainmenu();
        }

        remapMillis = millis();
    }
}

void idle(){
    if (idleMinutes != 0) {
        if ((millis() - pm) > idleMinutes*60000) bMax = 0;
        // Restore from EEPROM value here
        else bMax = EEPROM.read(1);
    }
}

void loop() {
    // Update key state to check for button presses
    checkKeys();
    // Make lights happen
    effects(10, ledMode);
    // Convert key presses to actual keyboard keys
    keyboard();
    idle();
    // Debug check for loops per second
#ifdef DEBUG
    serialDebug();
#endif
    serialCheck();
}
