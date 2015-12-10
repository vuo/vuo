This node set includes nodes for sending and receiving Art-Net messages, and for discovering available Art-Net devices.

[Art-Net](http://en.wikipedia.org/wiki/Art-Net) is a protocol for controlling stage lighting systems.  Typically a computer communicates via Ethernet with a device that converts Art-Net to/from a DMX512 signal, which is then connected to individual light fixtures, dimmers, or controllers.


## Art-Net protocol support

Vuo supports the following aspects of the Art-Net protocol:

   - Sending and receiving ArtDmx messages, on any of the 32,768 Net/Sub-Net/Universe combinations
   - Sending and receiving ArtPoll and ArtPollReply messages, to identify available devices on the network


## Troubleshooting

   - Art-Net uses the fast-but-unreliable UDP network protocol, so, to maximize performance, use it on a local wired network that's isolated from other traffic (e.g., Internet or local file transfers).
   - Some Art-Net-to-DMX512 converters fail to negotiate a network link when directly connected to a Mac Ethernet port.  Try connecting your Mac's Ethernet port to a 10/100 switch, then connect the 10/100 switch to the Art-Net-to-DMX512 converter.
   - Check for messages in the Console app.
