Aligns the layer with an edge or corner of a window.

For example, if the alignment "left" and "top" is chosen, the layer will be moved so its top left corner lines up with the top left corner of the window.

The `Horizontal Margin` and `Vertical Margin` ports adds space between the layer and the edge of the window.  When `Horizontal Alignment` is "left" or "right", `Horizontal Margin`'s value adds space left or right of the layer, respectively.  When `Vertical Alignment` is "top" or "bottom", `Vertical Margin`'s value adds space above or below the layer, respectively.

The layer you connect to this node should typically be at the origin (have center 0,0).  If the layer's center is not at the origin, its position is used to offset the layer after it is aligned.
