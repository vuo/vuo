Outputs *true* if the point is within the given layer. 

This node checks the point against the layer's position within the rendered layers (composite image). For example, if a layer is scaled and rotated before being rendered, then this node checks if the point falls within the layer's scaled and rotated bounds. 

   - `point` — The point to check, in Vuo coordinates. 
   - `renderedLayers` — The group of rendered layers containing the layer. 
   - `layerName` — The name of the layer. 
   - `withinLayer` — *True* if the layer is in the rendered layers and the point is in the layer. 
