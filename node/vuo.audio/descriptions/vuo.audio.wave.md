Calculates a set of values for a simple audio [waveform](http://en.wikipedia.org/wiki/Waveform).

Each time this node's refresh port receives an event, it will calculate a new set of values.  If you change `waveform` or `frequency`, the waveform changes smoothly to its new shape.

You can use this node to produce a tone at a specified pitch.  Connect the `Send Live Audio` node's `requestedChannels` output to this node's refresh port, then connect this node's `samples` output to the drawer attached to the `Send Live Audio` node.

   - `wave` — The shape of the waveform (sine, triangle, or sawtooth). 
   - `frequency` — The number of cycles per second (Hz) in the calculated waveform.  The musical note "A" has a frequency of <a href="https://en.wikipedia.org/wiki/A440_(pitch_standard)">around 440 Hz</a>.
   - `samples` — A set of calculated waveform values, ready to be filtered or output.
