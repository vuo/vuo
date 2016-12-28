These nodes are for sending and receiving [Open Sound Control (OSC)](http://opensoundcontrol.org/introduction-osc) messages. 

These nodes allow your composition to send messages to and receive messages from OSC-enabled devices and applications. Devices include sound synthesizers, stage lighting equipment, and control surfaces (including iPhone/iPad and Android apps). Applications include [TouchOSC](http://hexler.net/software/touchosc), [Lemur](http://liine.net/en/products/lemur/), and [Vezér](http://www.vezerapp.hu/).

To use a device or application to remote-control your composition, do one of the following: 

   - If the device or application supports <a href="http://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>, run the composition, then, in the device or application, select the Vuo composition from its list of available hosts. 
   - In the device or application, find the UDP port that it's sending on. In the composition, create a `Make OSC Input` node with that port number, and connect it to the `Receive OSC Messages` node.
   - In the composition, create a `Make OSC Input` node and set it to use a UDP port of your choice (typically 1024 or higher), then connect it to the `Receive OSC Messages` node. In the device or application, set it to send on that UDP port.

To use your composition to control a device or application, do one of the following:

   - If the device or application supports <a href="http://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>, run the composition, then, in the device or application, select the Vuo composition from its list of available hosts. 
   - In the device or application, find the UDP port that it's listening on. In the composition, create a `Make OSC Output` node with that port number, and connect it to the `Send OSC Messages` node.
   - To send to just a single device or application, find the IP address and UDP port that it's listening on. In the composition, create a `Make OSC IP Output` node with that IP address and port number, and connect it to the `Send OSC Messages` node.
   - In the composition, create a `Make OSC Output` node and set it to use a UDP port of your choice (typically 1024 or higher), then connect it to the `Send OSC Messages` node. In the device or application, set it to listen on that UDP port.

The **UDP port** is a number that represents the connection between the OSC sender and receiver. The sender and receiver both need to use the same UDP port. The UDP port is either set manually by the user or discovered automatically through Bonjour. 

Each OSC **message** contains an address and, optionally, one or more data values. 

The **address** is a piece of text beginning with `/`. Often, it looks like a file path or part of a URL, for example `/1/fader`. It may be used to describe how the data values should be interpreted, for example as positions of a fader control. 

Each **data value** is a simple piece of information (integer, real, boolean, or text). For example, a data value of 0.3 accompanying the address `/1/fader` could indicate that a fader control has been set to position 0.3. 

These nodes are based on the [OSC 1.0 specification](http://opensoundcontrol.org/spec-1_0). The following type tags are supported: nil (`N`), true (`T`), false (`F`), float32 (`F`), int32 (`I`), int64 (`H`), and OSC-string (`S`). 
