Nodes for sending and receiving [Open Sound Control (OSC)](http://opensoundcontrol.org/) messages.

These nodes allow your composition to send messages to and receive messages from OSC-enabled devices and applications. Devices include sound synthesizers, stage lighting equipment, and control surfaces (including iPhone/iPad and Android apps). Applications include [TouchOSC](http://hexler.net/software/touchosc), [Lemur](http://liine.net/en/products/lemur/), and [Vezér](http://www.vezerapp.hu/).

To use a device or application to remote-control your composition, do one of the following: 

   - If the device or application supports <a href="https://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>, run the composition, then, in the device or application, select the Vuo composition from its list of available hosts.
   - In the device or application, find the UDP port that it's sending on. In the composition, create a [Specify OSC Input](vuo-node://vuo.osc.make.input) node with that port number, and connect it to the [Receive OSC Messages](vuo-node://vuo.osc.receive2) node.
   - In the composition, create a [Specify OSC Input](vuo-node://vuo.osc.make.input) node and set it to use a UDP port of your choice (typically 1024 or higher), then connect it to the [Receive OSC Messages](vuo-node://vuo.osc.receive2) node. In the device or application, set it to send on that UDP port.

To use your composition to control a device or application, do one of the following:

   - If the device or application supports <a href="https://en.wikipedia.org/wiki/Bonjour_(software)">Bonjour</a>, run the composition, then, in the device or application, select the Vuo composition from its list of available hosts.
   - In the device or application, find the UDP port that it's listening on. In the composition, create a [Specify OSC Output](vuo-node://vuo.osc.make.output) node with that port number, and connect it to the [Send OSC Messages](vuo-node://vuo.osc.send) node.
   - To send to just a single device or application, find the IP address and UDP port that it's listening on. In the composition, create a [Specify OSC IP Output](vuo-node://vuo.osc.make.output.ip) node with that IP address and port number, and connect it to the [Send OSC Messages](vuo-node://vuo.osc.send) node.
   - In the composition, create a [Specify OSC Output](vuo-node://vuo.osc.make.output) node and set it to use a UDP port of your choice (typically 1024 or higher), then connect it to the [Send OSC Messages](vuo-node://vuo.osc.send) node. In the device or application, set it to listen on that UDP port.

The **UDP port** is a number that represents the connection between the OSC sender and receiver. The sender and receiver both need to use the same UDP port. The UDP port is either set manually by the user or discovered automatically through Bonjour. 

Each OSC **message** contains an address and, optionally, one or more data values. 

The **address** is a piece of text beginning with `/`. Often, it looks like a file path or part of a URL, for example `/1/fader`. It may be used to describe how the data values should be interpreted, for example as positions of a fader control.  When using the [Filter Address](vuo-node://vuo.osc.filter.address) node, Vuo supports case-sensitive wildcard matching.

Each **data value** is a simple piece of information (integer, real, boolean, or text). For example, a data value of 0.3 accompanying the address `/1/fader` could indicate that a fader control has been set to position 0.3. 

These nodes are based on the [OSC 1.0 specification](http://opensoundcontrol.org/).

### Data types

When receiving OSC values, the following type tags are supported: nil (`N`), true (`T`), false (`F`), float32 (`f`), float64/double (`d`), int32 (`i`), int64 (`h`), and OSC-string (`s`).

[Make Message](vuo-node://vuo.osc.message.make.1) and [Get Message Values](vuo-node://vuo.osc.message.get.1) nodes have one or more `Value` ports with changeable data types. By changing the data types (for example, by right-clicking on the port and selecting from the Set Data Type submenu), you can control the types of OSC data that the node sends or receives.

When you use a [Get Message Values](vuo-node://vuo.osc.message.get.1) node, Vuo converts the OSC data to each `Value` port's data type if possible.  For example, if a `Value` port is set to the Vuo type Real, then it can handle OSC data of type float64/double, float32, int64, int32, or string, converting it if necessary from an integer or text to a real number.

When sending OSC values, the same data types, except nil, are supported.  When you use a [Make Message](vuo-node://vuo.osc.message.make.1) node, Vuo chooses the OSC data type based on each `Value` port's data type together with the corresponding `Type` port's value.  When the `Type` port is set to `Auto`, Vuo uses the following mappings:

Vuo Type | OSC Data Type
-------- | -------------
Boolean  | true (`T`) or false (`F`)
Integer  | int64 (`h`)
Real     | float64/double (`d`)
Text     | OSC-string (`s`)

That is, Vuo sends 64-bit integer and real values by default.  To send 32-bit values, change the `Type` port to `Integer (32-bit)` or `Floating point (32-bit)` and set the `Value` port's type to `Integer` or `Real` accordingly.
