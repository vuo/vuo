Outputs the frequency associated with a MIDI note number. 

This node is useful for playing MIDI notes aloud in Vuo. You can connect this node to a `Make Audio Wave` node, then connect that to a `Send Live Audio` node. 

   - `Note Number` — Ranges from 0 to 127. 
   - `Frequency` — The frequency, in Hz, that corresponds to the note number, according to the [MIDI Tuning Standard](https://en.wikipedia.org/wiki/MIDI_Tuning_Standard). 

Examples: 

   - For note number 0 (C-1), the frequency is 8.18. 
   - For note number 60 (C4, middle C), the frequency is 261.63. 
   - For note number 69 (A4), the frequency is 440. 
   - For note number 127 (G9), the frequency is 12543.85. 
