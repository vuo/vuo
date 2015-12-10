Given a list of channel intensity values (received from a DMX device using the `Receive Art-Net Messages` node), creates a list of colors.

   - `DMX` — Some DMX channel intensity values to convert to colors.
   - `Color Map` — How to convert each set of DMX intensity values into a color:
      - Red, Green, Blue (RGB) — Each set of 3 input DMX channel values is interpreted as a single color
      - Red, Green, Blue, Amber (RGBA) — Each set of 4 input DMX channel values is interpreted as a single color
      - Red, Green, Blue, Amber, White (RGBAW) — Each set of 5 input DMX channel values is interpreted as a single color
      - Red, Green, Blue, White (RGBW) — Each set of 4 input DMX channel values is interpreted as a single color
      - Warm white, Cool white (WW/CW) — Each set of 2 input DMX channel values is interpreted as a single color
   - `Colors` — A list of colors corresponding with the input DMX values.

The amber, white, warm white, and cool white values are calculated based on how similar the input color is to each reference color.
