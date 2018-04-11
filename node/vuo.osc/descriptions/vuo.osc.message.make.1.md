Creates an OSC message with 1 data value.

Connect the output of this node to `Send OSC Message` to send the message on the network.

If you want to send data of type other than float64/double, unspecialize the `Data 1` port (right-click and choose Revert to Generic Data Type) and specialize it to the appropriate type (right-click and choose an item under Set Data Type).

See the [vuo.osc](vuo-nodeset://vuo.osc) documentation for information about how Vuo types are converted to OSC data types.
