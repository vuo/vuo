Outputs smoothed values from the faders of a Behringer BCF2000 MIDI/USB control surface.

When the composition starts, and when the `Reset` port receives an event, this node outputs the default values for each fader, and sends MIDI messages to the BCF2000 to physically position its motorized faders.

Connect a time source (such as the `Refreshed at Time` output from a [Fire on Display Refresh](vuo-node://vuo.event.fireOnDisplayRefresh) node) to this node's `Time` input port.  Events to the `Time` port pass through each output port when its fader value changes, until the smooth value change (configured via the fader's input port) has completed.
