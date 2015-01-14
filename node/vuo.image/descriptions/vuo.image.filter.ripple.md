Distorts the image with a wave or ripple effect.

This node applies sinusoidal ripples to the image. The ripples can be animated to move across the image by sending gradually increasing values to the `phase` port.

   - `angle` — The angle or direction of the ripples, in degrees. At 0, the ripples move side to side as `phase` increases. At 90, the ripples move up and down.
   - `amplitude` — The amount that each ripple distorts the image. At 0, there is no distortion.
   - `wavelength` — The size of each ripple, in Vuo coordinates. At 0, the output image is black.
   - `phase` — The current time in the wave cycle. At 2π (about 6.28), the phase is back to the beginning of the cycle.
