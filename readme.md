# Unified keypad code

This is a complete rewrite of the firmware intended
for ALL PC keypads. The goals of this newer codebase are as follows:

- [x] Replace the neopixel and dotstar libraries with
FastLED as it is SIGNIFICANTLY faster.
- Support input methods:
    - [x] direct
    - [ ] matrix
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
- [ ] Add profile switcher as an alternative to the layer shifter for models like the macropad.
- [ ] Add the ability to map one key to multiple (change EEPROM index to 10 pages per key or something like that)

