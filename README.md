AnimatedGIFs
============
This [SmartMatrix Library](http://docs.pixelmatix.com/SmartMatrix/index.html) example displays 32x32 GIF animations loaded from a SD Card connected to the Teensy 3.1 using the [SmartMatrix Shield](http://docs.pixelmatix.com/SmartMatrix/shieldref.html)

The GIFs can be up to 32 pixels in width and height.  This code has been tested with 32x32 pixel and 16x16 pixel GIFs, but is optimized for 32x32 pixel GIFs.

Wiring is on the default Teensy 3.1 SPI pins, and chip select can be on any GPIO (customize by defining SD_CS in the code)

Function     | Pin
-------------|----
DOUT         |  11
DIN          |  12
CLK          |  13
CS (default) |  15

This code first looks for .gif files in the /gifs/ directory (you can customize this with the GIF_DIRECTORY definition) then plays random GIFs in the directory, looping each GIF for DISPLAY_TIME_SECONDS

This example is meant to give you an idea of how to add GIF playback to your own sketch.  For a project that adds GIF playback with other features, take a look at [Light Appliance](https://github.com/CraigLindley/LightAppliance) and [Aurora](https://github.com/pixelmatix/aurora):

If you find any GIFs that won't play properly, please attach them to a new
Issue post in the GitHub repo [here](https://github.com/pixelmatix/AnimatedGIFs/issues):
