Creates a layer that can be combined with other layers to create a composite image. 

The image is drawn with a shadow behind it.  You can control the shadow's color, focus, and placement.

The image used to create this layer will be rendered at its original size and rotation in the composite image. If you want to scale and rotate the layer, use the `Make Scaled Layer with Shadow` node instead.

   - `name` — A name to identify the layer, allowing other nodes to select it from a group of layers. 
   - `image` — The image displayed in the layer. For the layer to be visible, you have to provide an image. 
   - `center` — The center point of the image, in Vuo coordinates.  The center point is snapped so that it aligns exactly with a pixel.
   - `alpha` — The image's opacity, from 0 (fully transparent) to 1 (fully opaque). 
   - `shadowColor` — The color of the layer's shadow.  Use the color's alpha value to control the intensity of the shadow.
   - `shadowBlur` — The amount to blur the layer's shadow, in pixels.  The blur extends this many pixels inward and this many pixels outward along the edge of the shadow. 
   - `shadowAngle` — The shadow's angle relative to the layer, in degrees. At 0, the shadow is to the right of the layer. At 90, the shadow is above the layer. 
   - `shadowDistance` — The distance that the shadow is offset from the layer, in Vuo coordinates. 
