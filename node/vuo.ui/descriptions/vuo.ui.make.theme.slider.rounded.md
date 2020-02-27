Creates a theme to be used when rendering sliders.

   - `Label Font` — The font to use for the slider's `Label` text.
   - `Label Color`, `Hovered` — The colors for the slider's `Label` text, respectively, when the mouse is not hovering over it, and when the mouse is hovering over it.  (The `Label Font` also specifies a color; that color is multiplied by each of these colors.)
   - `Handle Width`, `Height` — The size of the drag handle, in Vuo Coordinates.  (When the slider is vertical, the handle is rotated 90°, swapping the Width and Height.)
   - `Handle Border Thickness` — The size of the drag handle's outline, in Vuo Coordinates.
   - `Handle Corner Roundness` — How rounded the corners of the drag handle are.  A value of 0 means the corners are sharp, producing a rectangle; a value of 1 means the corners are fully rounded, producing a circle (if the width and height are equal) or a capsule (if the width and height differ).
   - `Handle Color`, `Hovered`, `Pressed` — The colors for the drag handle, respectively, when the mouse is not hovering over the slider, when the mouse is hovering over it, and when the handle is being dragged.
   - `Handle Border Color`, `Hovered`, `Pressed` — The colors for the drag handle's outline.
   - `Track Depth` — The height of the track when horizontal, or the width of the track when vertical, in Vuo Coordinates.
   - `Track Border Thickness` — The size of the track's outline, in Vuo Coordinates.
   - `Track Corner Roundness` — How rounded the corners of the track are.
   - `Active Track Color`, `Hovered` — The colors for the portion of the track less than the slider's current value (left of the drag handle when horizontal; below the drag handle when vertical).
   - `Active Track Border Color`, `Hovered` — The colors for the outline of the portion of the track less than the slider's current value.
   - `Inactive Track Color`, `Hovered` — The colors for the portion of the track greater than the slider's current value.
   - `Inactive Track Border Color`, `Hovered` — The colors for the outline of the portion of the track greater than the slider's current value.
   - `Margin between Track and Label` — The space between the bottom of the track and its `Label` text, in Vuo Coordinates.

The track length for each slider is set by the [Make Slider](vuo-node://vuo.ui.make.slider) node.
