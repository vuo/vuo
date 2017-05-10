Outputs smoothed values from the foot controls of a Behringer BCF2000 MIDI/USB control surface.

When the composition starts, and when the `Reset` port receives an event, this node outputs the default values for each foot control, and sends MIDI messages to the BCF2000 to set each foot control's initial state.

Connect a time source (such as the `Requested Frame` output from a `Render Scene/Layers/Image to Window` or `Fire on Display Refresh` node) to this node's `Time` input port.  Events to the `Time` port pass through each output port when its foot control value changes, until the smooth value change (configured via the foot control's input port) has completed.
