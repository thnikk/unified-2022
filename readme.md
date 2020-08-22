# Unified keypad code

This is a complete rewrite of the firmware intended
for ALL PC keypads. The goals of this newer codebase are as follows:

- [x] Replace the neopixel and dotstar libraries with
FastLED as it is SIGNIFICANTLY faster.
- Support input methods:
    - [x] direct
    - [ ] matrix
        - This will probably be done later when I make a real matrix model for real (like a 3x3.)
    - [x] touch
- [x] Generally reduce size and increase efficiency.
- [x] Switch to patched version of HID Project library for NKRO support (enabling the 7K for use with this code.)
- [x] Unify electronics for future versions (SAMD21 Mini.)
- [x] Use serial communication for all configuration.
- [x] Use side button as modifier key for more possible keys.
- [x] Add idle mode.
    - [x] Make idle mode timeout configurable in configurator.
- [x] Fix remapper to allow for non-printable keys.
- [x] Allow HID library to accept ASCII values instead of requiring names.
- [x] Add profile switcher as an alternative to the layer shifter for models like the macropad (allow for more than 2 layers.)
    - [x] Change LED color while switching profiles to indicate the active profile.
- [x] Add option to disable the side button.
- [ ] Add the ability to map one key to multiple (change EEPROM index to 10 pages per key or something like that)
    - This can cause some issues and may be best to omit for minimizing confusion. Using F13-F24 and mapping macros through AHK is a much more versatile solution, but may cause issues with anti-cheat software (which is malware anyway.)
- [x] Add mouse support
    - [x] Add MB4 and MB5
- [x] Add LED effects for remapping (color for layer and white for currently mapping key.)
- [x] Support for RGBW LEDs (not yet implemented within the FastLED library)

# Maintenance
- [x] Clean up EEPROM addresses.
- [ ] Clean up LED functions.
- [ ] Remove printBlock function.
- [ ] Test everything.
- [ ] Fix legacy 7k not properly restoring the active profile.
    - This seems specific to profile mode, as layer mode is unaffected.
