Outputs *true* if the point is within the given layer. 

This node checks the point against the layer's position within the rendered layers (composite image). For example, if a layer is scaled and rotated before being rendered, then this node checks if the point falls within the layer's scaled and rotated bounds. 

   - `Point` — The point to check, in Vuo Coordinates. 
   - `Rendered Layers` — The group of rendered layers containing the layer. 
   - `Layer Name` — The name of the layer. 
   - `Within Layer` — *True* if the layer is in the rendered layers and the point is in the layer. 
