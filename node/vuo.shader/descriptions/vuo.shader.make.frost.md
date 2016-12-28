Creates a graphics shader that can paint a 3D object like frosted glass.

This shader takes a picture of what's been rendered so far, then paints this 3D object with a distorted version of that picture.

   - `Color` — The color to tint the object.
   - `Brightness` — How bright to make the object.  When Brightness is 1 and Color is white, the scene's colors are passed through as-is, without tinting or change in brightness.  You'll often want to make this greater than 1, to compensate for the loss in brightness when tinting with a non-white color.
   - `Noise Time` — The time at which to evaluate the noise.  To animate the noise, connect a continuously increasing number, such as the output of the `Render Scene to Window` node's `Requested Frame` port.
   - `Noise Amount` — How much the light gets bent when passing through this 3D object.
   - `Noise Scale` — The size of the noise pattern.  At smaller values, the noise ripples are more closely packed together.
   - `Chromatic Aberration` — How much the red/green/blue color channels separate when the light is bent.  At 0, all colors are treated the same.  At larger values, red and blue are bent differently than green, simulating the prismatic effect of a typical lens.
   - `Iterations` — How many different noise patterns are averaged together.  A value of 1 uses just a single noise pattern, and can look like a warped piece of plastic.  Higher values produce images that look more like frosted glass, but can take longer to render.

This shader ignores lighting (though any lighting in the scene behind this 3D object is preserved).
