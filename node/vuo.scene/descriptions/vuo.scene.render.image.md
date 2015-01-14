Creates an image that is a snapshot of a 3D scene.

   - `objects` — The 3D objects to place in the scene.
   - `cameraName` — The name of the camera through which the scene is viewed. The camera is selected from any in `objects`. If no matching camera is found, then a camera from `objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `width`, `height` — The width and height of the resulting image, in pixels.
   - `image` — The snapshot image of the scene. If no scene object is visible in part of the image, that part of the image will be transparent.
   - `depthImage` — A counterpart to the snapshot image. The depth image represents the distance between the camera and objects in the scene. Objects closer to the camera are darker, objects further from the camera are lighter. 
