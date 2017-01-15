Calculates a set of values for a simple audio [waveform](http://en.wikipedia.org/wiki/Waveform).

Each time this node's refresh port receives an event, it will calculate a new set of values.  If you change `Waveform` or `Frequency`, the waveform changes smoothly to its new shape.

You can use this node to produce a tone at a specified pitch.  Connect the `Send Live Audio` node's `Requested Channels` output to this node's refresh port, then connect this node's `Samples` output to the drawer attached to the `Send Live Audio` node.

   - `Wave` — The shape of the waveform (sine, triangle, or sawtooth). 
   - `Frequency` — The number of cycles per second (Hz) in the calculated waveform.  The musical note "A" has a frequency of <a href="https://en.wikipedia.org/wiki/A440_(pitch_standard)">around 440 Hz</a>.
   - `Set Phase` — Changes the current position in the waveform.  At 1, the phase is back to the beginning of the cycle.  This is useful for [controlling the interference between multiple waves](http://web.archive.org/web/20031009204604/http://www.soundonsound.com/sos/aug99/articles/synthsecrets.htm).
   - `Samples` — A set of calculated waveform values, ready to be filtered or output.
