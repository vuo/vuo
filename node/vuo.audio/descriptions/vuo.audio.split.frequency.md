Separates audio into multiple channels by frequency. 

This node is useful for visualizing or processing audio based on ranges of pitches. 

This node splits a single channel into a list of channels, each composed (mostly) of frequencies within a distinct range. (Since the splitting is done using audio filters with a gradual slope, some frequencies outside the range are present at lower amplitudes.) 

   - `Cutoff Frequencies` — The borderlines between frequency ranges, in Hz. The number of items in `Split Samples` is one more than the number of items in `Cutoff Frequencies`. 
   - `Split Samples` — The split channels, in ascending order of frequency. 

As an example, a list of 3 frequency cutoff values — 300, 1200, 4000 — will result in 4 output channels: 

   - A channel with frequencies below 300 Hz (created with a low-pass filter).
   - A channel with frequencies between 300 Hz and 1200 Hz (created with a band-pass filter). 
   - A channel with frequencies between 1200 Hz and 4000 Hz (created with a band-pass filter). 
   - A channel with frequencies above 4000 Hz (created with a high-pass filter). 
