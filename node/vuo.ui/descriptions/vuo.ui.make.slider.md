Creates a layer to interactively set a value by dragging a handle along a track.

   - `Window` — The window in which the layer is rendered. Connect this to the [Render Layers to Window](vuo-node://vuo.layer.render.window2) node's `Updated Window` output port.
   - `Label` — The text to show beneath the slider.
   - `Set Value` — Sets the initial value of the slider, or changes its current value.
   - `Range` — The minimum and maximum values the slider encompasses.
   - `Orientation` — Whether the handle moves left-to-right or bottom-to-top.
   - `Position` — The slider's position, in Vuo Coordinates.
   - `Track Length` — The size of the track (not including border), in Vuo Coordinates.
   - `Theme` — Information about the slider's appearance.  See the [Make Slider Theme (Rounded)](vuo-node://vuo.ui.make.theme.slider.rounded) node.
