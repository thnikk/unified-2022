// Libraries
#include <Arduino.h>
#include <Bounce2.h>
#include <HID-Project.h>
#include <FastLED.h>
// Pins, mappings, and board-specific libraries in this file
#include <models.h>

// Initialize inputs and LEDs
Bounce * bounce = new Bounce[numkeys+1];
CRGBArray<numkeys> leds;

// buffer for keypresses
bool pressed[numkeys+1];
bool lastPressed[numkeys+1];

// Starting and max brightness
uint8_t b = 127;
uint8_t bMax = b;

// Default LED mode and speed of effects
uint8_t ledMode = 0;
uint8_t effectSpeed = 10;

// Colors for custom LED mode
// These are the initial values stored before changed through the remapper
uint8_t custColor[] = {224,192,224,192,224,192,224};

// BPS
uint8_t bpsCount;

// FreeTouch
int touchValue;
const int touchThreshold = 800;

// Default idle time
byte idleMinutes = 5;

// Millis timer for idle check
unsigned long pm;

// Version (Update this value to update EEPROM for AVR boards)
uint8_t version = 2;

// Display names for each key (in specific order, do not re-arrange)
const String PROGMEM friendlyKeys[] = {
    "L_CTRL", "L_SHIFT", "L_ALT", "L_GUI", "R_CTRL", "R_SHIFT",
    "R_ALT", "R_GUI", "ESC", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
    "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",
    "F21", "F22", "F23", "F24", "ENTER", "BACKSP", "TAB", "PRINT", "PAUSE", "INSERT",
    "HOME", "PAGE_UP", "DELETE", "END", "PAGE_DN", "RIGHT", "LEFT", "DOWN", "UP",
    "PAD_DIV", "PAD_MULT", "PAD_SUB", "PAD_ADD", "PAD_ENT", "PAD_1", "PAD_2", "PAD_3",
    "PAD_4", "PAD_5", "PAD_6", "PAD_7", "PAD_8", "PAD_9", "PAD_0", "MENU",
    "V_MUTE", "V_UP", "V_DOWN"
};

// Check if any key has been pressed in the loop.
bool anyPressed = 0;

void eepromInit(){
    // If version from EEPROM/flash doesn't match,
    // reset EEPROM values
    if (EEPROM.read(0) != version){
        EEPROM.write(0, version);
        // Assign default values
        EEPROM.write(1, bMax);
        b = bMax;
        EEPROM.write(2, ledMode);
        EEPROM.write(3, idleMinutes);
        for (uint8_t x=0; x<numkeys; x++) EEPROM.write(4+x, custColor[x]);
        for (uint8_t y=0; y<2; y++) { // Mapping
            for (uint8_t x=0; x<numkeys; x++) {
                int address=20+(y*30)+x;
                EEPROM.write(address, mapping[y][x]);
            }
        }
        // Write values
        COMMIT
    }
    // Otherwise, restore values
    else {
        bMax = EEPROM.read(1);
        ledMode = EEPROM.read(2);
        idleMinutes = EEPROM.read(3);
        for (uint8_t x=0;x<numkeys;x++){
            custColor[x] = EEPROM.read(4+x);
        }
        for (uint8_t y=0; y<2; y++) {
            for (uint8_t x=0; x<numkeys; x++) {
                int address=20+(y*30)+x;
                mapping[y][x] = EEPROM.read(address);
            }
        }
    }
}

void eepromUpdate(){
    // If values don't match, update them
    if (bMax != EEPROM.read(1)) EEPROM.write(1, bMax);
    if (ledMode != EEPROM.read(2)) EEPROM.write(2, ledMode);
    if (idleMinutes != EEPROM.read(3)) EEPROM.write(3, idleMinutes);
    for (uint8_t x=0; x<numkeys; x++) if (custColor[x] != EEPROM.read(3+x)) EEPROM.write(3+x, custColor[x]);
    for (uint8_t y=0; y<2; y++) { // Mapping
        for (uint8_t x=0; x<numkeys; x++) {
            int address=20+(y*30)+x;
            if (mapping[y][x] != EEPROM.read(address)) EEPROM.write(address, mapping[y][x]);
        }
    }
    COMMIT
}

void setup() {
    // Set the serial baudrate
    Serial.begin(9600);

    // Start LEDs
    FastLED.addLeds<NEOPIXEL, NPPIN>(leds, numkeys);
#if defined (ADAFRUIT_TRINKET_M0)
    FastLED.addLeds<DOTSTAR, INTERNAL_DS_DATA, INTERNAL_DS_CLK, BGR>(ds, 1);
#endif

    // Set brightness
    FastLED.setBrightness(255);

    // Initialize EEPROM
    eepromInit();

#ifndef TOUCH
    // Set pullups and attach pins to debounce lib with debounce time (in ms)
    for (uint8_t x=0; x<=numkeys; x++) {
        pinMode(pins[x], INPUT_PULLUP);
        bounce[x].attach(pins[x]);
        bounce[x].interval(20);
    }
#else
    qt_1.begin();
    qt_2.begin();
#if numkeys == 4
    qt_3.begin();
    qt_4.begin();
#endif
#endif

    // Start freetouch for side button
#ifndef AVR
    qt.begin();
#endif

    NKROKeyboard.begin();
}

#if defined (TOUCH)
// Touch models need to manually have each pressed value assigned.
void checkTouch() {
    touchValue = qt_1.measure();
    if (touchValue > 500) pressed[0] = 0;
    else if ( touchValue < 450 ) pressed[0] = 1;
    touchValue = qt_2.measure();
    if (touchValue > 500) pressed[1] = 0;
    else if ( touchValue < 450 ) pressed[1] = 1;
#if numkeys == 4
#endif
    // Always update anyPressed
    for(uint8_t x=0; x<=numkeys; x++) if (!pressed[x]) anyPressed = 1;
}
#else
// Regular models can just iterate with a for loop.
void checkSwitch() {
    for(uint8_t x=0; x<=numkeys; x++){
        bounce[x].update();
        pressed[x] = bounce[x].read();
        if (!pressed[x]) anyPressed = 1;
    }
}
#endif

unsigned long checkMillis;
// All inputs are processed into pressed array
void checkState() {
    // Limiting input polling to polling rate increases speed by 17x!
    if ((millis() - checkMillis) >= 1) {
        // Write bounce values to main pressed array
        anyPressed = 0;

        // Use models.h defined check function
        CHECK

#ifndef AVR
        // Get current touch value and write to main array
        touchValue = qt.measure();
        // If it goes over the threshold value, the button is pressed
        if (touchValue > touchThreshold) pressed[numkeys] = 0;
        // To release, it must go under the threshold value by an extra 50 to avoid constantly changing
        else if ( touchValue < touchThreshold-50 ) pressed[numkeys] = 1;
#endif
        checkMillis=millis();
    }
}

// Check x key state
void keyCheck(uint8_t x) {
    Serial.print("Key ");
    Serial.print(x+1);
    Serial.print(" has been ");
    if (pressed[x] == 0) Serial.println("pressed.");
    if (pressed[x] == 1) Serial.println("released.");
}


uint8_t lastLayer;
// Compares inputs to last inputs and presses/releases key based on state change
void keyboard() {

    // If the side button is held, release any keys that are held down.
    uint8_t layer = pressed[numkeys];
    if (lastLayer != layer) {
        NKROKeyboard.releaseAll();
        lastLayer = layer;
    }

    for (uint8_t x=0; x<numkeys; x++){
        // If the button state changes, press/release a key.
        if ( pressed[x] != lastPressed[x] ){
#ifdef DEBUG
            keyCheck(x); // Only prints on state change
#endif
            if (!pressed[x]) bpsCount++;
            pm = millis();
            uint8_t key = mapping[!layer][x];

            // Check press state and press/release key
            switch(key){
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
                default: if (!pressed[x]) KBP(key); if (pressed[x]) KBR(key); break;
            }
            // Save last pressed state to buffer
            lastPressed[x] = pressed[x];
        }
    }
}

// Cycle through rainbow
void wheel(){
    static uint8_t hue;
    for(uint8_t i = 0; i < numkeys; i++) {
        float z = i;
        if (pressed[i]) leds[i] = CHSV(hue+(z*64),255,255);
        else leds[i] = 0xFFFFFF;
    }
#if defined (ADAFRUIT_TRINKET_M0)
    if (anyPressed == 0) ds[0] = CHSV(hue,255,255);
    else ds[0] = 0xFFFFFF;
#endif
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
        uint8_t z = ledMap[i]; // Allows custom LED mapping (for grids)
        leds[z] = CHSV(hue+(i*50),sat[i],val[i]);
    }
#if defined (ADAFRUIT_TRINKET_M0)
    static int satDS;
    static int valDS;
    if (anyPressed) {
        if (satDS < 255) satDS = satDS+8;
        if (satDS > 255) satDS = 255; // Keep saturation within byte range
        if (satDS == 255 && valDS > 0) valDS = valDS-8;
        if (valDS < 0) valDS = 0; // Same for val
    }
    else { satDS=0; valDS=255; }
    ds[0] = CHSV(hue,satDS,valDS);
#endif
    hue-=8;
    if (hue < 0) hue = 255;
    FastLED.show();
}

// Custom colors
void custom(){
    for(int i = 0; i < numkeys; i++) {
        uint8_t z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(custColor[z],255,255);
        else leds[z] = 0xFFFFFF;
    }
#if defined (ADAFRUIT_TRINKET_M0)
    // Set DotStar to left key color
    if (anyPressed == 0) ds[0] = CHSV(custColor[0],255,255);
    else ds[0] = 0xFFFFFF;
#endif
    FastLED.show();
}

unsigned long avgMillis;
uint8_t bpsColor;
uint8_t lastColor;
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
        uint8_t z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(lastColor+100,255,255);
        else leds[z] = 0xFFFFFF;
    }
#if defined (ADAFRUIT_TRINKET_M0)
    if (anyPressed == 0) ds[0] = CHSV(lastColor+100,255,255);
    else ds[0] = 0xFFFFFF;
#endif
    FastLED.show();

}

unsigned long effectMillis;
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
        Serial.println(count);
        count = 0;
        speedCheckMillis = millis();
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

void keyTable() {
    // Print welcome message
    remapExp();
    uint8_t lineLength = 0;
    // Print top line of table
    for (int y = 0; y < 69; y++) Serial.print("-");
    Serial.println();
    for (int x = 0; x <= 66; x++) {
        if (lineLength == 0) Serial.print("| ");
        // Make every line wrap at 30 characters
        uint8_t nameLength = friendlyKeys[x].length(); // save as variable within for loop for repeated use
        lineLength += nameLength + 6;
        Serial.print(friendlyKeys[x]);
        nameLength = 9 - nameLength;
        for (nameLength; nameLength > 0; nameLength--) { // Print a space
            Serial.print(" ");
            lineLength++;
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
    for (uint8_t x=0; x<=66;x++) {
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
            Serial.print(F("Current values: "));
            for (uint8_t x=0;x<numkeys;x++) {
                Serial.print(custColor[x]);
                if (x != numkeys-1) Serial.print(", ");
            }
            break;
        case 5: // Remapper
            // Print key names and numbers
            Serial.println();
            Serial.println(F("Current values: "));
            for (uint8_t y=0;y<2;y++) {
                Serial.print("Layer ");
                Serial.print(y+1);
                Serial.print(": ");
                for (uint8_t x=0;x<numkeys;x++) { keyLookup(mapping[y][x]); if (x<numkeys-1) Serial.print(", "); }
                Serial.println();
            }
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
            if (value >= 0 && value <= 66) return 128+value;
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
        for(uint8_t x=0;x<numkeys;x++){
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

// Menu for remapping
void remapMenu(){
    // Main loop
    while(true){
    uint8_t layer;
    Serial.println(F("Which layer would you like to remap?"));
    Serial.println(F("0 to exit"));
    Serial.println(F("1 to remap layer 1"));
    Serial.println(F("2 to remap layer 2"));
    printBlock(5);

    // Select layer
    bool layerCheck = 0;
    while(layerCheck == 0){
        int incomingByte = Serial.read();
        if (incomingByte > 0){
            if (incomingByte>=48&&incomingByte<=50) {
                layer = incomingByte-48;
                if (layer == 0) layerCheck = 1; // Return before printing if layer is 0
                else layerCheck = 1;
            }
            else Serial.println(F("Please enter a valid value."));
        }
    }

    // Exit if user inputs 0
    if (layer == 0) return;

    // Remap selected layer
    keyTable();
    Serial.print(F("Layer "));
    Serial.print(layer);
    Serial.print(F(": "));
    for(uint8_t x=0;x<numkeys;x++){
        uint8_t key = parseKey();
        keyLookup(key);
        // Subtract 1 because array is 0 indexed
        mapping[layer-1][x] = key;
        // Separate by comma if not last value
        if (x<numkeys-1) Serial.print(F(", "));
        // Otherwise, create newline
        else Serial.println();
    }
    Serial.println();
    }
}

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
                default:
                    Serial.println(F("Please enter a valid value."));
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
        if (Serial && set == 0) { printBlock(0); set=1; }
        // If closed, reset greet message.
        if (!Serial) set = 0;

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
    checkState();
    // Make lights happen
    effects(10, ledMode);
    // Convert key presses to actual keyboard keys
    keyboard();
    idle();
    // Debug check for loops per second
    //speedCheck();
#ifndef DEBUG
    serialCheck();
#endif
}
