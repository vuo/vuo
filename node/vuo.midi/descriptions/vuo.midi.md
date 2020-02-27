Nodes for sending and receiving MIDI notes and other MIDI messages (MIDI events).

A MIDI **input device** (such as a keyboard, controller, or sequencer) can send MIDI messages to the [Receive MIDI Events](vuo-node://vuo.midi.receive) node, allowing the device to control the composition.

A MIDI **output device** (such as a synthesizer, sequencer, or stage lighting equipment) can receive MIDI messages from the [Send MIDI Event](vuo-node://vuo.midi.send) node, allowing the composition to control the device.

Each MIDI device has a unique **ID**, which is assigned by the operating system. If all devices were plugged in after the computer started up, then ID 0 is usually the first device plugged in, ID 1 is usually the second device plugged in, etc.

Each MIDI device has a **name**, which is not necessarily unique. On macOS, you can look up a device's name in the Audio MIDI Setup application.

A MIDI **note** message often represents a musical note. It includes:

   - A **note number**, ranging from 0 to 127. This often represents the pitch of the note, with Middle C (C4) at 60. 
   - A **velocity**, ranging from 0 to 127. This often represents the force with which the note is played. 
   - Whether this is a **Note On** or a **Note Off** message. A Note On message represents that a note is pressed (started). A Note Off message represents that a note is released (ended).

A MIDI **controller** message often represents a musical effect (volume, panning, filter cutoff, sustain, etc.). A controller message can be sent by a control on a MIDI device (knob, fader, pedal, etc.). It includes:

   - A **controller number**, ranging from 0 to 127. This often represents the type of effect. 
   - A **controller value**, ranging from 0 to 127. This often represents amount of the effect. Some effects use the whole range of values, while others are either on (0-63) or off (64-127).

A MIDI **pitch bend** message often represents a change in the relative pitch of all notes on a certain MIDI channel.  Its value ranges from 0 to 16,383, with 8,192 being the center point (no pitch change).  The pitch bend range varies per device and configuration, but often the full range covers a whole tone (value 0 is 2 semitones down, value 16,383 is 2 semitones up).

MIDI note, controller, and pitch bend messages also include:

   - A **channel**, ranging from 1 to 16. Each channel has its own stream of MIDI notes and controller values. A channel often represents one musical instrument.

You can send MIDI messages between two Vuo compositions, one with a [Send MIDI Event](vuo-node://vuo.midi.send) node and the other with a  [Receive MIDI Events](vuo-node://vuo.midi.receive) node. For this to work, you need to set up a MIDI device that supports both input and output. The compositions will communicate through this device. On macOS, you can set up the device like this: open the Audio MIDI Setup application and go to the MIDI window, double-click on the IAC driver, and check the box for "Device is online". Then run the compositions.
