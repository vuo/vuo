Creates an image that is a snapshot of a 3D scene.

   - `Objects` — The 3D objects to place in the scene.
   - `Camera Name` — The name of the camera through which the scene is viewed. The camera is selected from any in `Objects`. If no matching camera is found, then a camera from `Objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `Width`, `Height` — The width and height of the resulting image, in pixels.
   - `Color Depth` — The number of bits to use to represent each color channel in the output image; higher values are more precise but use more graphics storage space.  Images are typically 8 bits per channel ("8bpc"), though if you're working with image feedback, you may want to use higher precision.
   - `Image` — The snapshot image of the scene. If no scene object is visible in part of the image, that part of the image will be transparent.
   - `Depth Image` — A counterpart to the snapshot image. The depth image represents the distance between the camera and objects in the scene. Objects closer to the camera are darker, objects further from the camera are lighter. 
