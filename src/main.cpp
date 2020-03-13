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
#include <Keyboard.h>
#include <FastLED.h>

// Define pins for dotstar
#define DATAPIN    7
#define CLOCKPIN   8

// Initialize inputs and LEDs
Bounce * bounce = new Bounce[numkeys+1];
Adafruit_FreeTouch qt = Adafruit_FreeTouch(4, OVERSAMPLE_8, RESISTOR_50K, FREQ_MODE_NONE);
CRGBArray<numkeys> leds;
CRGB ds[1];

const byte pins[] = { 0, 2, 20, 19 };
byte mapping[][4] = {
{177,96,122,120},
{218,217,216,215},
};

bool pressed[numkeys+1];
bool lastPressed[numkeys+1];

byte b = 127;
byte bMax = 255;

// FreeTouch
int touchValue;
int touchThreshold = 500;

// Millis timer for idle check
unsigned long pm;

const byte gridMap[] = {0, 1, 3, 2};

void setup() {
    // Set the serial baudrate
    Serial.begin(115200);

    // Start LEDs
    FastLED.addLeds<NEOPIXEL, 1>(leds, numkeys);
    FastLED.addLeds<DOTSTAR, INTERNAL_DS_DATA, INTERNAL_DS_CLK, BGR>(ds, 1);

    // Set brightness
    FastLED.setBrightness(255);

    // Map last key to Esc
    //mapping[numkeys] = char(177);

    // THIS SECTION NEEDS TO BE CHANGED FOR FULL FREETOUCH SUPPORT

    // Set pullups and attach pins to debounce lib with debounce time (in ms)
    for (byte x=0; x<=numkeys; x++) {
        pinMode(pins[x], INPUT_PULLUP);
        bounce[x].attach(pins[x]);
        bounce[x].interval(20);
    }

    // Start freetouch for side button
    qt.begin();

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
        for (byte x=0;x<=numkeys;x++) {
            if (!pressed[x]){
                pm = millis();
                Serial.print("Button ");
                Serial.print(x);
                Serial.println("is pressed.");
            }
        }
        checkMillis=millis();
    }
}

// Compares inputs to last inputs and presses/releases key based on state change
void keyboard() {
    for (byte x=0; x<numkeys; x++){
        // If the button state changes, press/release a key.
        if ( pressed[x] != lastPressed[x] ){
            if (!pressed[x]) Keyboard.press(mapping[!pressed[numkeys]][x]);
            if (pressed[x]) Keyboard.release(mapping[!pressed[numkeys]][x]);
            lastPressed[x] = pressed[x];
        }
    }
}

void wheel(){
    static uint8_t hue;
    bool facePress = 0;
    for(int i = 0; i < numkeys; i++) {
        byte z = gridMap[i];
        if (pressed[i]) leds[z] = CHSV(hue,255,255);
        else {
            leds[z] = 0xFFFFFF;
            facePress = 1;
        }
    }
    if (facePress == 0) ds[0] = CHSV(hue,255,255);
    else ds[0] = 0xFFFFFF;
    hue++;
    FastLED.show();
}

unsigned long effectMillis;
byte effectSpeed = 10;
void effects(byte speed) {
    // All LED modes should go here for universal speed control
    if ((millis() - effectMillis) > speed){
        wheel();

        if (b < bMax) b++;
        if (b > bMax) b--;
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
const String greet="Press 0 to enter the main menu.";
const String menu[]={
    "Press 0 to enter the remapper.",
    "Press 1 to set the LED mode.",
    "Press 2 to set the brightness.",
    "Press 3 to set the custom colors."
};

byte step;
void mainmenu() {
    int incomingByte = Serial.read();
    step = 0;
    if (incomingByte == 122) {
        while(true){
            effects(5);
            if (step == 0) for (byte x=0;x<4;x++) Serial.println(menu[x]);
            step = 1;
            incomingByte = Serial.read();
            //Serial.println(greet);
            if (incomingByte == 120) return;
        }
    }
}

unsigned long remapMillis;
void serialCheck() {
    if ((millis() - remapMillis) > 10){
        if (Serial.available() > 0) mainmenu();
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
    // Convert keypresses to actual keyboard keys
    keyboard();
    // Make lights happen
    effects(10);
    idle();
    // Debug check for loops per second
    //speedCheck();

    //serialCheck();
}
