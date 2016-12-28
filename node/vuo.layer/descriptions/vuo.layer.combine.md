Groups multiple layers together and applies a transformation to the group. 

This node is useful for making a group of layers all move, rotate, or resize together. 

   - `Transform` — A transform that changes the layer group's translation, rotation, or scale. It should use Vuo Coordinates. For any Real-Size layers in the group, rotating or scaling the group will affect the layer's position but not its rotation or size. 
   - `Layers` — The layers to put into a group. When the layers are rendered, they're placed on top of each other in the order listed, with the first layer on the bottom and the last layer on the top. 
