Converts text to speech.

Enter some text, connect the output of this node to [Send Live Audio](vuo-node://vuo.audio.send2), and fire an event into the `Speak` port.

   - `Text` — The words to speak.
   - `Voice` — The name of the voice to use.  You can install additional voices by opening System Preferences and selecting Accessibility > Speech > System Voice > Customize.
   - `Words per Minute` — How quickly to speak.
   - `Pitch` — How high or low the voice sounds, in hertz.  Pitches are snapped to equal-tempered musical notes.  For some voices, pitches other than `Auto` may have no effect or cause distortion.
   - `Modulation` — How much inflection to use when speaking.  For some voices, modulation has no effect.

Changes to the voice and its parameters take effect the next time the `Speak` port receives an event.
