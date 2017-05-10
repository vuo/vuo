Outputs smoothed values from the matrix of buttons on the right side of a Behringer BCF2000 MIDI/USB control surface.

When the composition starts, and when the `Reset` port receives an event, this node outputs the default values for each button, and sends MIDI messages to the BCF2000 to set each button's LED.

Connect a time source (such as the `Requested Frame` output from a `Render Scene/Layers/Image to Window` or `Fire on Display Refresh` node) to this node's `Time` input port.  Events to the `Time` port pass through each output port when its button value changes, until the smooth value change (configured via the button's input port) has completed.
