Analyzes audio to determine its tempo, and outputs a clock and beat events in sync with the audio.

It may take a few seconds to analyze the audio before the tempo, beat, and clock are determined.

   - `Time` — The time at which to analyze the audio.  Connect a `Requested Frame` output to this port.
   - `Tempo Range` — The range of tempos that you expect the audio's tempo to fall within.
   - `Samples` — The audio samples to analyze.
   - `Nudge` — When this port receives an event, it shifts the timing of the output `Beat` port by a sixteenth note.  Use this if the output isn't quite in sync with the audio.
   - `Reset` — Clears beat analysis history, and resets the output `Clock` to 0.  Use this when the song or tempo changes, or if the correct beat hasn't been determined after a while.
   - `Beats per Minute` — The analyzed tempo of the audio input.
   - `Beat` — Outputs an event at the time of each quarter note.  This port filters events coming into the `Time` input — events that occur on the beat are passed through; events that don't are blocked.  Use `Nudge` if the output isn't quite in sync with the audio.
   - `Clock` — Outputs a timestamp normalized to each quarter note.  When the beat is first detected, this port outputs 0.  After that, it increases by 1 for each quarter note, and it increases fractionally between quarter notes.
