Calculates a set of values for a simple audio [waveform](https://en.wikipedia.org/wiki/Waveform).

Each time this node's `Calculate Samples` port receives an event, it will calculate a new set of values.  If you change `Wave` or `Frequency`, the waveform changes smoothly to its new shape.

You can use this node to produce a tone at a specified pitch.  Connect the [Fire at Audio Rate](vuo-node://vuo.audio.fireAtBufferRate) node's output to this node's `Calculate Samples` port, then connect this node's `Samples` output to the drawer attached to the [Send Live Audio](vuo-node://vuo.audio.send2) node.

   - `Calculate Samples` — Generate a new set of audio samples.
   - `Wave` — The shape of the waveform (sine, triangle, or sawtooth).
   - `Frequency` — The number of cycles per second (Hz) in the calculated waveform.  The musical note <b>A<sub>4</sub></b> has a frequency of <a href="https://en.wikipedia.org/wiki/A440_(pitch_standard)">around 440 Hz</a>.
   - `Set Phase` — Changes the current position in the waveform.  At 1, the phase is back to the beginning of the cycle.  This is useful for [controlling the interference between multiple waves](https://web.archive.org/web/20031009204604/http://www.soundonsound.com/sos/aug99/articles/synthsecrets.htm).
   - `Samples` — A set of calculated waveform values, ready to be filtered or output.
