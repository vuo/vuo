Given a list of colors, creates a list of channel intensity values which can be sent to a DMX device using the `Send Art-Net Messages` node.

   - `Colors` — Some colors to convert to DMX.
   - `Color Map` — How to convert each color into DMX intensity values:
      - Red, Green, Blue (RGB) — Each input color becomes 3 DMX channel values
      - Red, Green, Blue, Amber (RGBA) — Each input color becomes 4 DMX channel values
      - Red, Green, Blue, Amber, White (RGBAW) — Each input color becomes 5 DMX channel values
      - Red, Green, Blue, White (RGBW) — Each input color becomes 4 DMX channel values
      - Warm white, Cool white (WW/CW) — Each input color becomes 2 DMX channel values
   - `DMX` — A list of integer intensity values ranging from 0 to 255.

The amber, white, warm white, and cool white values are calculated based on how similar the input color is to each reference color.
