Creates an OSC message with 4 data values.

Connect the output of this node to `Send OSC Message` to send the message on the network.

If you want to send data of types other than float64/double, unspecialize the corresponding `Data` input ports (right-click and choose Revert to Generic Data Type) and specialize them to the appropriate types (right-click and choose an item under Set Data Type).

See the [vuo.osc](vuo-nodeset://vuo.osc) documentation for information about how Vuo types are converted to OSC data types.
