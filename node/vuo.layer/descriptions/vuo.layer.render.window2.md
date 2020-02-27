Displays a window containing a composite image. 

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

   - `Layers` — The layers that make up the composite image. The layers are placed in the order listed, with the first layer in back and the last layer in front.
   - `Multisampling` — How smooth edges of layers are.
      - A value of 1 means each pixel is evaluated once — no smoothing.
      - A value of 2, 4, or 8 means each pixel that lies on the edge of a layer is sampled multiple times and averaged together, to provide a smoother appearance.
   - `Set Window Description` — A specification of the appearance of the window or the way the user can interact with it.
   - `Updated Window` — Fires an event with information about the window whenever the state of the window changes. You can send this information to [vuo.mouse](vuo-nodeset://vuo.mouse), [vuo.ui](vuo-nodeset://vuo.ui), and other nodes so they can respond to changes in the composite image, window status, and user interactions.

### Antialiasing

If the graphics rendered by this node look jagged or blocky, you can…

   - Smooth the edges of layers by increasing `Multisampling`.
   - Smooth the interiors of image layers by using [Improve Downscaling Quality](vuo-node://vuo.image.mipmap).
   - Smooth an exported movie with the Antialiasing setting under File > Export > Movie…

### Retina

See the [Image node set documentation](vuo-nodeset://vuo.image) for information on rendering Real Size layers on Retina and non-Retina screens.
