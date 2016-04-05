This node set includes nodes for sending and receiving data through an RS232 serial device, and for discovering available RS232 serial devices.

[RS232 Serial](https://en.wikipedia.org/wiki/Serial_port) is a protocol for communicating between physical devices.  These days, RS232 serial communication typically happens through a USB cable.

With this node set, your composition can interact with (for example) —

   - The [Arduino](http://www.arduino.cc/) microcontroller — receive data from sensors, and send data to control LEDs and motors
   - Some modern flat-screen televisions — switch channels, change the volume, go to sleep and wake up
   - Some modern network infrastructure equipment (routers, switches, firewalls)
   - Using a USB-to-RS232 adaptor — computers, mice, printers, cameras, barcode scanners, GPS receivers, modems, and teletypes from the 1980s and 1990s

The `Receive Serial Data` and `Send Serial Data` nodes receive and send pieces of **data** (information encoded as numbers). The amount of data is measured in **bytes**.

If a serial device sends or receives data in the form of UTF-8 or ASCII encoded text, then you can connect a `Convert Data to Text` or `Convert Text to Data` node to your `Receive Serial Data` or `Send Serial Data` node in order to translate between data and text. (Currently, these are the only built-in nodes for converting between serial data and Vuo's data types.)

When receiving data from a serial device, the data may arrive as a series of small chunks. For example, if a serial device sends the text "Hello world", the `Receive Serial Data` might fire a series of events with data representing the text "He", "llo w", "orld". You can put these chunks back together using the `Split Text Stream` node.


## Troubleshooting

If you're not sure whether Vuo is detecting your serial device:

   - Add a `Receive Serial Data` node to your composition. Double-click on its `Device` input port to bring up a menu of all connected serial devices. See if your device is in the list.
   - Check the Console application for any messages about serial devices.

If Vuo isn't detecting your device:

   - Make sure your composition is referring to the correct device. (Sometimes two devices of the same model can have different names and URLs.)
   - Make sure you've installed any necessary drivers for the device. (On Mac OS 10.8 and earlier, you may need to install [FTDI's VCP driver](http://www.ftdichip.com/Drivers/VCP.htm).)
   - Make sure the device is securely plugged in.
   - Try plugging the device into a different port on your computer.

If Vuo detects your device, but doesn't seem to be receiving or sending any data:

   - Make sure your composition is referring to the correct device, especially if you have multiple serial devices connected.
   - Make sure that another Vuo composition isn't already connected to the device. (Because of the way serial devices work, only one Vuo composition at a time can use a given serial device.)
   - Try unplugging the device and plugging it back in. (If another Vuo composition was using the device but isn't using it anymore, this will make your current composition retry connecting to the device.)

If Vuo detects your device, but the data received or sent seems garbled or corrupted:

   - Make sure that another application isn't already connected to the device. (Because of the way serial devices work, if different applications are trying to communicate with the same device simultaneously, the data can get mixed up.)
   - Make sure that you're not sending more data than the device can handle with each `Send Serial Data` event. Try breaking down the data into smaller pieces before sending it.
