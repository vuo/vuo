Creates an OSC message with a list of data values.

All OSC data values in the list must be of the same type.  To create an OSC message with data values of multiple types, use the `Make Message (1–11)` nodes.

Connect the output of this node to [Send OSC Message](vuo-node://vuo.osc.send) to send the message on the network.

See the [vuo.osc](vuo-nodeset://vuo.osc) documentation for information about how Vuo types are converted to OSC data types.
