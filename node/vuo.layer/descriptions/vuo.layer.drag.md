Fires events when the mouse tries to drag a layer. 

This node outputs the position that a layer would be dragged to, but does not actually move the layer. To move the layer, connect this node's `Dragged Center To` output port to the `Center` input port of the node that creates the layer (such as `Make Layer`). 

This node checks the mouse pointer against the layer's position within the rendered layers (composite image). For example, if a layer is scaled and rotated before being rendered, then this node would check if the mouse pointer falls within the layer's scaled and rotated bounds. (However, the output of this node would be the center of the un-scaled and un-rotated layer.) 

This node tracks drags within the window that renders the layers. For this node to work, the `Rendered Layers` input port must receive its data from a node that renders to a window (such as `Render Layers to Window`), not a node that renders to some other destination (such as `Render Layers to Image`). 

   - `Rendered Layers` — The group of rendered layers containing the layer. 
   - `Layer Name` — The name of the layer. 
   - `Button` — The mouse button(s) to track. 
   - `Modifier Key` — The keyboard button (if any) that must be held for the mouse action to be tracked. 
   - `Started Drag` — When the mouse button is pressed within the layer, fires an event with the layer's center point. 
   - `Dragged Center To` — When the mouse is dragged and the drag started within the layer, fires an event with the center point that the layer would be dragged to. 
   - `Ended Drag` — When the mouse button is released for a drag that started within the layer, fires an event with the center point that the layer would be dragged to. 
