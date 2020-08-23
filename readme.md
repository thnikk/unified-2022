# Unified keypad code

This is a complete rewrite of the firmware for ALL PC keypads. It's intended to be faster, more optimized, and more universal for all current and future models.

### Features

- [x] Uses FastLED instead of slower NeoPixel and DotStar libraries (also allows for easier programming of color changes.)
- [ ] Support input methods:
    - [x] direct
    - [ ] matrix
        - This will probably be done later when I make a real matrix model for real (like a 3x3.)
    - [x] touch
- [x] Uses patched version of HID Project library for NKRO support and additional mappable keys.
- [x] Uses serial communication for all configuration.
- [x] Idle mode with configurable timeout
- [x] 3 modes for the side button:
    1) Layer shift
    2) Profile switch
    3) Off
- [ ] Multi-key mapping (ie. mapping key 1 to ctrl+alt+delete.)
    - This will use a lot of EEPROM space, and also creates a bit of confusion. F13-F24 + AutoHotKey may be the better way to go, but I'd like to keep this implemented for those that already use it.
- [x] Mouse button support for M1-M5 keys
- [x] LEDs will change color while remapping to reflect the current key being remapped and the current profile.
- [x] Support for RGBW LEDs (not yet implemented within the FastLED library)

# Installation


### Download
Download and install VS Code from the link here:

[Visual Studio Code](https://code.visualstudio.com/download)

You'll also need Git so download and install it from here. Leave all defaults selected during installation.

[Git](https://git-scm.com/download)

You also need to download the code from this repository. This can be done by clicking on the `code` button on the top right and selecting "Download ZIP". Extract the zip to somewhere for later.

![image](https://thnikk.moe/img/docs/program/ghDownload.png)

Install the PlatformIO extension
--------------------------------
PlatformIO is what converts the code into something the keypad can understand. To install it, open VS Code and click on the icon with the 3 connected squares and one floating square in the side bar.

![image](https://thnikk.github.io/images/rst/program/extension.png)

From here, you can type "platformio" into the search bar and click the install button Platformio IDE. Restart if it prompts you to.

![image](https://thnikk.github.io/images/rst/program/pio.png)

After platformio is installed, please reboot your computer.

Uploading the code to the keypad
--------------------------------
Now you can go to File>Open Folder and select the folder you extracted from the GitHub download. The folder you open should contain another folder called src.

![image](https://thnikk.github.io/images/rst/program/folder.png)

Now you can click on the PlatformIO logo (the ant head) in the side bar. If you have anything starting with "Env" under project tasks, click the one for your corresponding model to expand the menu.

![image](https://thnikk.github.io/images/rst/program/upload.png)

Now click upload and you should see a terminal pop up on the bottom of the screen start spitting out information. When it's done, you should see the environment you selected with "SUCCESS" next to it, meaning your keypad has been programmed!

![image](https://thnikk.github.io/images/rst/program/terminal.png)

If the build fails, you should be able to scroll up to see an error message in red. If you can't figure out what the problem is (ones related to upload port usually mean the keypad isn't plugged in or you're using a bad USB cable,) let me know and I'll do my best to help you.

