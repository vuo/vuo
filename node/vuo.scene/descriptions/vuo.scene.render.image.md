Creates an image that is a snapshot of a 3D scene.

   - `objects` — The 3D objects to place in the scene.
   - `width`, `height` — The width and height of the resulting image, in pixels.
   - `image` — The snapshot image of the scene. If no scene object is visible in part of the image, that part of the image will be transparent.
   - `depthImage` — A counterpart to the snapshot image. The depth image represents the distance between the camera and objects in the scene. Objects closer to the camera are darker, objects further from the camera are lighter. 
