Displays a window containing a 3D scene.

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

   - `Objects` — The 3D objects to place in the scene.
   - `Set Window Description` — A specification of the appearance of the window or the way the user can interact with it.
   - `Camera Name` — The name of the camera through which the scene is viewed. The camera is selected from any in `Objects`. If no matching camera is found, then a camera from `Objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `Multisampling` — How smooth edges of 3D objects are.
      - A value of 1 means each pixel is evaluated once — no smoothing.
      - A value of 2, 4, or 8 means each pixel that lies on the edge of an object is sampled multiple times and averaged together, to provide a smoother appearance.
   - `Updated Window` — Fires an event with information about the window whenever the state of the window changes. You can send this information to [vuo.mouse](vuo-nodeset://vuo.mouse), [vuo.ui](vuo-nodeset://vuo.ui), and other nodes so they can respond to changes in window status and user interactions.

### Antialiasing

If the graphics rendered by this node look jagged or blocky, you can…

   - Smooth the edges of objects by increasing `Multisampling`.
   - Smooth the image materials of objects by using [Improve Downscaling Quality](vuo-node://vuo.image.mipmap).
   - Smooth an exported movie with the Antialiasing setting under File > Export > Movie…
