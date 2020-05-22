/*
This is a complete rewrite of the firmware intended
for ALL PC keypads. The main goals are as follows:

- Replace the neopixel and dotstar libraries with
FastLED as it is SIGNIFICANTLY faster.
- Add a HAL for managing inputs from different
methods like direct, matrix, and touch.
- Generally reduce size and increase efficiency.

Smaller feature goals are:
- Add idle mode
- Buffered LED effects for easier transitions

*/

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
//CRGB ds[1];

const byte pins[] = { 12, 11 };
uint8_t mapping[][2] = {
{122,120},
{177,KEY_F13}
};

bool pressed[numkeys+1];
bool lastPressed[numkeys+1];

byte b = 127;
byte bMax = 255;

byte LEDmode = 3;
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

            // Check press state and press/release key
            switch(key){
                // Key exceptions need to go here for NKROKeyboard
                case 177:
                    if (!pressed[x]) NKROKeyboard.press(KEY_ESC);
                    if (pressed[x]) NKROKeyboard.release(KEY_ESC);
                    break;
                case KEY_F13:
                    if (!pressed[x]) NKROKeyboard.press(KEY_F13);
                    if (pressed[x]) NKROKeyboard.release(KEY_F13);
                    break;
                default:
                    if (!pressed[x]) NKROKeyboard.press(key);
                    if (pressed[x]) NKROKeyboard.release(key);
                    break;
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
// Red:0, Orange:32, Yellow:64, Green:96, Aqua:128, Blue:160, Purple:192, Pink:224
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
void effects(byte speed) {
    // All LED modes should go here for universal speed control
    if ((millis() - effectMillis) > speed){
        // Select LED mode
        switch(LEDmode){
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
const String greet="Press 0 to enter the main menu.";
const String menu[]={
    "Press 0 to enter the remapper.",
    "Press 1 to set the LED mode.",
    "Press 2 to set the brightness.",
    "Press 3 to set the custom colors."
};

byte step;
void mainmenu() {
    int incomingByte = SerialUSB.read();
    step = 0;
    if (incomingByte == 48) {
        while(true){
            effects(5);
            if (step == 0) for (byte x=0;x<4;x++) SerialUSB.println(menu[x]);
            step = 1;
            incomingByte = SerialUSB.read();
            //SerialUSB.println(greet);
            if (incomingByte == 120) return;
        }
    }
}

unsigned long remapMillis;
void serialCheck() {
    if ((millis() - remapMillis) > 10){
        if (SerialUSB.available() > 0) mainmenu();
        remapMillis = millis();
    }
}

void idle(){
    if ((millis() - pm) > 60000) bMax = 0;
    else bMax = 255;
}

void loop() {
    // Update key state to check for button presses
    checkState();
    // Convert key presses to actual keyboard keys
    keyboard();
    // Make lights happen
    effects(10);
    idle();
    // Debug check for loops per second
    speedCheck();
    //serialCheck();
}
