Displays a window containing an image.

When the composition starts or this node is added to a running composition, it displays a window that contains a graphics area. The window provides a menu option to toggle between windowed and full-screen mode.

   - `Image` — The image to draw in the graphics area. The window is resized to fit the image (unless the input to `Set Window Description` says otherwise).
   - `Set Window Description` — A specification of the appearance of the window or the way the user can interact with it.
   - `Updated Window` — Fires an event with information about the window whenever the state of the window changes. You can send this information to [vuo.mouse](vuo-nodeset://vuo.mouse), [vuo.ui](vuo-nodeset://vuo.ui), and other nodes so they can respond to changes in window status and user interactions.

### Window size

Usually, the window is resized to fit the image, and the window's aspect ratio is kept the same as the image's when the user resizes it.

The exception is when the input to `Set Window Description` has come from [Change Window Size](vuo-node://vuo.window.size2) or [Lock Window Aspect Ratio](vuo-node://vuo.window.aspectRatio2) — in which case [Unlock Window Aspect Ratio](vuo-node://vuo.window.aspectRatio.reset2) will return the window to its original aspect ratio and resume automatic resizing.

### Retina
This node renders images at the same physical size on both Retina and non-Retina screens.  See the [Image node set documentation](vuo-nodeset://vuo.image) for information on how the image's Scale Factor affects rendering on Retina and non-Retina screens.

On Retina screens, 1x images may appear slightly blurry.  To render those images at Retina resolution, use [Change Window Size](vuo-node://vuo.window.size2) to resize the window to half of the image's size in pixels.

### Antialiasing

If the graphics rendered by this node look jagged or blocky, you can…

   - Smooth the image using [Improve Downscaling Quality](vuo-node://vuo.image.mipmap).
   - Smooth an exported movie with the Antialiasing setting under File > Export > Movie…
