AnimatedGIFs
============
This [SmartMatrix Library](http://docs.pixelmatix.com/SmartMatrix/index.html) example displays 32x32 GIF animations loaded from a SD Card connected to the Teensy 3.1 using the [SmartMatrix Shield](http://docs.pixelmatix.com/SmartMatrix/shieldref.html)

The GIFs must be 32x32 pixel resolution exactly

Wiring is on the default Teensy 3.1 SPI pins, and chip select can be on any GPIO (customize by defining SD_CS in the code)

Function     | Pin
-------------|----
DOUT         |  11
DIN          |  12
CLK          |  13
CS (default) |  15

This code first looks for .gif files in the /gifs/ directory (you can customize this with the GIF_DIRECTORY definition) then plays random GIFs in the directory, looping each GIF for DISPLAY_TIME_SECONDS

This example is meant to give you an idea of how to add GIF playback to your own sketch.  For a project that adds GIF playback with other features, take a look at [Light Appliance](https://github.com/CraigLindley/LightAppliance):

If you find any 32x32 GIFs that won't play properly, please attach them to a new
Issue post in the GitHub repo [here](https://github.com/pixelmatix/AnimatedGIFs/issues):

## Libraries
In addition to the [SmartMatrix Library](http://docs.pixelmatix.com/SmartMatrix/index.html), this sketch requires the [SdFat Library](https://github.com/greiman/SdFat) which can be tricky to install correctly.

[This tutorial](https://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use/) explains the basics of installing libraries.

### Troubleshooting
Seeing this error means the SdFat library isn't installed (correctly):
`fatal error: SdFat.h: No such file or directory`

It's possible the SdFat library folder is nested.  Find SdFat.h in the folder where the SdFat library is installed and make sure the path looks like this: `libraries\SdFat\SdFat.h`, not like this: `libraries\SdFat\SdFat\SdFat.h`

It's also possible that the SdFat library is installed in a folder with characters the Arduino IDE doesn't like.  If you download the SdFat .zip from GitHub and try to add it as is, it will be in a folder called `SdFat-master`, and Arduino doesn't like the dash character, and it will also be nested.  Following the steps in the Adafruit tutorial avoids both of these issues.
