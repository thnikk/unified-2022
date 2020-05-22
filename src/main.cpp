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
    }
    // Add extra line break
    SerialUSB.println();
}

const String modeNames[]={ "Cycle", "Reactive", "Custom", "BPS" };
void ledMenu() {
    printBlock(2);
    while(true){
        int incomingByte = SerialUSB.read();
        if (incomingByte>=48 && incomingByte<=57) {
            ledMode = incomingByte-48;
            SerialUSB.print("Selected ");
            SerialUSB.println(modeNames[ledMode]);
            SerialUSB.println();
            return;
        }
        else if (incomingByte > 0) { SerialUSB.println("Please enter a valid value."); }
    }
}

String inString = "";    // string to hold input
int parseByte(){
    while (true) {
    int incomingByte = SerialUSB.read();
    if (incomingByte > 0) {
        // Parse input
        //SerialUSB.println(incomingByte);
        if (isDigit(incomingByte)) {
          // convert the incoming byte to a char and add it to the string:
          inString += (char)incomingByte;
        }
        // if you get a newline, print the string, then the string's value:
        if (incomingByte == '\n') {
          int value = inString.toInt();
          //SerialUSB.println(value);
          if (value >= 0 && value <= 255) { return value; }
          else SerialUSB.println("Please enter a valid value.");
          // clear the string for new input:
          inString = "";
        }
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

byte step;
void mainmenu() {
    while(true){
        int incomingByte = SerialUSB.read();
        effects(5, 0);
        if (step == 0) {
            printBlock(1);
        }
        step = 1;
        incomingByte = SerialUSB.read();
        // Exit
        switch(incomingByte-48){
            case(0):
                SerialUSB.println("Settings saved! Exiting...");
                printBlock(0);
                step = 0;
                pm = millis(); // Reset idle counter post-config
                return;
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
        }
        //if (incomingByte > 0 && incomingByte != 48) printBlock(1);
    }
}

bool set;
unsigned long remapMillis;
void serialCheck() {
    // Check for serial connection every 10ms
    if ((millis() - remapMillis) > 10){
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
    else bMax = 255;
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
    //speedCheck();
    serialCheck();
}
