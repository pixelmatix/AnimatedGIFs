AnimatedGIFs Decoder Library
============================

## Update November 2020

**This sketch has been adapted into the [GifDecoder library](https://github.com/pixelmatix/GifDecoder/), which uses a more efficient GIF decoder.  This sketch won't be maintained anymore, but you can find similar functionality in the examples in GifDecoder**

-----

This library decodes Animated GIF frames from a Filesystem, and draws them to a display.  The Filesystem and Display specific functions have been abstracted out using callback functions contained in the sketch, so this library should be useful on many different platforms.

A [SmartMatrix Library](http://docs.pixelmatix.com/SmartMatrix/index.html) example is included, reading from a SD card.  This works on both the Teensy and ESP32 platforms.

For ESP32/ESP8266 SPIFFS, try adapting the SmartMatrix example, using the FilenameFunctions code in this repository: https://github.com/prenticedavid/AnimatedGIFs_SD

For Adafruit's Arcada platform, see the examples in this repository: https://github.com/adafruit/Adafruit_Arcada_GifDecoder

For other platforms or other displays, start with the SmartMatrix example, and remove the SmartMatrix-specific code, and modify FilenameFunctions as needed.

The included example is meant to give you an idea of how to add GIF playback to your own sketch.  For a project that adds GIF playback with other features, take a look at [Light Appliance](https://github.com/CraigLindley/LightAppliance), [Aurora](https://github.com/pixelmatix/aurora), and the Adafruit Arcada examples.

If you find any GIFs that won't play properly, please attach them to a new
Issue post in the GitHub repo [here](https://github.com/pixelmatix/AnimatedGIFs/issues):

Many thanks to David Prentice and Adafruit for improvements on the original AnimatedGIFs sketch, and turning the sketch into a library, as well as the original author Craig A. Lindley.
