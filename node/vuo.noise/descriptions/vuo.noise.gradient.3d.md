Generates a pseudorandom number using Perlin noise or simplex noise.

This node can be used to animate an object in a natural-looking motion by sending gradually changing values to the `position` input port. 

   - `position` – For a given position, the gradient noise value is always the same. Typically you would want to send into this input port a series of numbers, with each number differing from the previous one by a small amount (less than 1). When the number is an integer, the gradient noise value is 0.
   - `gradientNoise` — The way to generate the gradient noise.
      - "Perlin" — improved Perlin noise
      - "Simplex" — simplex noise
   - `value` — The gradient noise value, usually ranging from -1 to 1.
