Creates a solid-color rounded rectangle layer that can be combined with other layers to create a composite image. 

   - `Name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `Color` — The color that fills the rectangle.
   - `Center` — The center point of the rectangle, in Vuo Coordinates.
   - `Rotation` — The rectangle's rotation counterclockwise, in degrees.
   - `Width` — The width of the rectangle, in Vuo Coordinates.
   - `Height` — The height of the rectangle, in Vuo Coordinates.
   - `Sharpness` — How sharp the edges of the rectangle are.  A value of 0 means the edges are blurry; a value of 1 means the edges are sharp.
   - `Roundness` — How rounded the corners of the rectangle are.  A value of 0 means the corners are sharp, producing a rectangle; a value of 1 means the corners are fully rounded, producing a circle (if the width and height are equal) or a capsule (if the width and height differ).
