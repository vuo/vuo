Distorts the object with a wave effect.

This node displaces the object's vertices with a wave in the direction specified by the `Displacement` port — either **transverse** (perpendicular to the travel of the wave) or **longitudinal** (parallel to the travel of the wave).

The ripples can be animated to move across the object by sending gradually increasing values to the `Phase` port.

   - `Angle` — The direction the wave travels through the object, in degrees.
   - `Amplitude` — The amount to distort the object. At 0, there is no distortion.
   - `Wavelength` — The size of each wave, in Vuo Coordinates.
   - `Phase` — The current time in the wave cycle. At 1, the phase is back to the beginning of the cycle.
   - `Dispersion` — The pattern in which the wave travels — linear or radial.
   - `Displacement` — The direction in which the wave displaces the object's vertices — transverse or longitudinal.
