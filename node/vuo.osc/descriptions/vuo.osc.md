These nodes are for receiving [Open Sound Control (OSC)](http://opensoundcontrol.org/introduction-osc) messages. 

These nodes allow your composition to receive messages from OSC-enabled devices and applications. Devices include sound synthesizers, stage lighting equipment, and control surfaces. Applications include [TouchOSC](http://hexler.net/software/touchosc), [Lemur](http://liine.net/en/products/lemur/), and [Vezér](http://www.vezerapp.hu/). With these nodes, you can use a device or application (including an iPhone/iPad or Android app) to remote-control your composition. 

To connect your composition to an OSC-enabled device or application, do one of the following: 

   - If the device or application supports <a href="http://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>, run the composition, then, in the device or application, select "Vuo OSC Server" from its list of available hosts. 
   - In the device or application, find the UDP port that it's sending on. In the composition, set the `Receive OSC Messages` node to use that UDP port. 
   - In the composition, set the `Receive OSC Messages` node to use a UDP port of your choice (typically 1024 or higher). In the device or application, set it to send on that UDP port. 

The **UDP port** is a number that represents the connection between the OSC sender and receiver. The sender and receiver both need to use the same UDP port. The UDP port is either set manually by the user or discovered automatically through Bonjour. 

Each OSC **message** contains an address and, optionally, one or more data values. 

The **address** is a piece of text beginning with `/`. Often, it looks like a file path or part of a URL, for example `/1/fader`. It may be used to describe how the data values should be interpreted, for example as positions of a fader control. 

Each **data value** is a simple piece of information (integer, real, boolean, or text). For example, a data value of 0.3 accompanying the address `/1/fader` could indicate that a fader control has been set to position 0.3. 

These nodes are based on the [OSC 1.0 specification](http://opensoundcontrol.org/spec-1_0). The following type tags are supported: nil (`N`), true (`T`), false (`F`), float32 (`F`), int32 (`I`), int64 (`H`), and OSC-string (`S`). 
