Gives the CMYK (cyan, magenta, yellow, key/black) values of a color.

   - `Colorspace` —
      - Linear — A simple linear inversion formula (`(1-R-K)/(1-K)`) is used to convert sRGB to CMYK.
      - Apple — The conversion is performed using a device-independent ICC color-matching profile provided by Apple.  This may give more realistic results than Linear.
   - `Cyan`, `Magenta`, `Yellow — Ranges from 0 to 1, where 1 is fully cyan, magenta, or yellow.
   - `Black` — Ranges from 0 (full color) to 1 (completely black).
