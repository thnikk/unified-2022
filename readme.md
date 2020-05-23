# Unified keypad code

This is a complete rewrite of the firmware intended
for ALL PC keypads. The goals of this newer codebase are as follows:

- [x] Replace the neopixel and dotstar libraries with
FastLED as it is SIGNIFICANTLY faster.
- [ ] Add a HAL for managing inputs from different
methods like direct, matrix, and touch.
- [x] Generally reduce size and increase efficiency.
- [x] Switch to patched version of HID Project library for NKRO support (enabling the 7K for use with this code.)
- [x] Unify electronics for future versions (SAMD21 Mini.)
- [x] Use serial communication for all configuration.
- [x] Use side button as modifier key for more possible keys.
- [x] Add idle mode.
    - [ ] Make idle mode timeout configurable.
- [ ] Fix remapper to allow for non-printable keys.
- [ ] Allow HID library to accept ASCII values instead of requiring names.
    - Both of the above could be fixed by using the default Arduino library, but it doesn't support NKRO.
