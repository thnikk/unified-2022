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
- [x] Fix remapper to allow for non-printable keys.
- [x] Allow HID library to accept ASCII values instead of requiring names.

## Program size for AVR boards

The biggest issue at the moment is that the FastLED library and the configurator code both take up a lot of program space. This is only an issue for the 7K keypad not using the newer SAMD21 mini, so I can either limit features for the older model or try to reduce code size wherever I can. Ideally there would be a dedicated configurator program (not Termite) that could receive a code from the keypad and display text so it doesn't need to be stored as a string on the keypad itself, so that may be the next project.
