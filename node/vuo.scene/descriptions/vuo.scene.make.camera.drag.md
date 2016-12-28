Creates a draggable perspective camera that can be added to a 3D scene.

   - Drag with the **left** mouse button to **orbit** (move the camera spherically around the pivot).
   - Drag with the **middle** mouse button to **pan** (move the camera sideways or up and down).
   - Drag with the **right** mouse button to **roll** (rotate the camera right or left).
   - **Scroll** to **dolly** (move the camera closer or farther away).

This node lets you easily, interactively navigate through a 3D scene.

   - `Name` — The camera's name, to be set on `Render Scene to Window` nodes.
   - `Modifier Key` — If set, only accept mouse input when this modifier key is pressed.
   - `Window` — Which window is this scene object rendering to?  Used to calculate how much of an effect mouse drags should have.
   - `Field of View` — The camera's [angle of view](http://en.wikipedia.org/wiki/Angle_of_view), in degrees. 
   - `Distance Min`, `Distance Max` — The minimum and maximum distance in front of the camera at which an object is visible (clipping planes), in Vuo Coordinates. These should be greater than 0. 
   - `Reset` — Set the camera's transform back to its default position.
