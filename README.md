AnimatedGIFs
============
This [SmartMatrix Library](http://docs.pixelmatix.com/SmartMatrix/index.html) example displays GIF animations loaded from a SD Card connected to the Teensy 3 using the [SmartMatrix Shield](http://docs.pixelmatix.com/SmartMatrix/shieldref.html)

The example can be modified to drive displays other than SmartMatrix by replacing SmartMatrix Library calls in `setup()` and the `*Callback()` functions with calls to a different library

Wiring is on the default Teensy 3 SPI pins, and chip select can be on any GPIO (customize by defining `SD_CS` in the code).  For Teensy 3.5/3.6 with the onboard SDIO, change `SD_CS` to `BUILTIN_SDCARD`

Function     | Pin
-------------|----
DOUT         |  11
DIN          |  12
CLK          |  13
CS (default) |  15

Wiring for ESP32 follows the default for the ESP32 SD Library, see: https://github.com/espressif/arduino-esp32/tree/master/libraries/SD

This code first looks for .gif files in the /gifs/ directory (you can customize this with the GIF_DIRECTORY definition) then plays random GIFs in the directory, looping each GIF for DISPLAY_TIME_SECONDS

This example is meant to give you an idea of how to add GIF playback to your own sketch.  For a project that adds GIF playback with other features, take a look at [Light Appliance](https://github.com/CraigLindley/LightAppliance) and [Aurora](https://github.com/pixelmatix/aurora):

If you find any GIFs that won't play properly, please attach them to a new
Issue post in the GitHub repo [here](https://github.com/pixelmatix/AnimatedGIFs/issues):
