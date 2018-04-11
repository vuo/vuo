Changes the darker colors in the image to transparent. 

Any colors in the image that fall below the given threshold are changed to transparent (masked). The threshold may be based on the color's luminance; its amount of red, green, or blue; or its opacity. 

   - `Threshold` — The cutoff point between dark (0) and light (1). Any colors that fall below this threshold are changed to transparent.
   - `Threshold Type` — The measure of darkness or lightness to use.  See `Make Grayscale Image` for further information.
   - `Sharpness` — How sharp the edges of masked areas will be. A value of 0 means the transition to transparent is very gradual; a value of 1 means the transition is immediate. 
