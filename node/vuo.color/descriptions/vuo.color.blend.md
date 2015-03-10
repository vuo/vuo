Blends two colors into a single color. 

   - `background` — The background color (bottom layer) to blend.
   - `foreground` — The foreground color (top layer) to blend.
   - `blendMode` — The way that the colors should be blended. For information about blend modes, see: 
      - [Blend modes on Wikipedia](http://en.wikipedia.org/wiki/Blend_modes)
      - [Photoshop blend modes](http://help.adobe.com/en_US/photoshop/cs/using/WSfd1234e1c4b69f30ea53e41001031ab64-77eba.html)
      - [GIMP blend modes](http://docs.gimp.org/en/gimp-concepts-layer-modes.html)
   - `foregroundOpacity` — The opacity that the foreground color should have in the blended color, ranging from 0 to 1.

Thanks to [Romain Dura](http://mouaif.wordpress.com/2009/01/05/photoshop-math-with-glsl-shaders/) for reference implementations of many blend modes.
