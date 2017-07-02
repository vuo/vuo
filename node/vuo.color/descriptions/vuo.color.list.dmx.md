Given a list of colors, creates a list of channel intensity values which can be sent to a DMX device using the `Send Art-Net Messages` node.

   - `Colors` — Some colors to convert to DMX.
   - `Color Map` — How to convert each color into DMX intensity values.  The number of color channels determines the number of DMX channels used — for example, in RGBAW mode each input color becomes 5 DMX channel values, and in Cyan, Magenta, Yellow mode, 3.
   - `DMX` — A list of integer intensity values ranging from 0 to 255.

The amber, white, warm white, and cool white values are calculated based on how similar the input color is to each reference color.
