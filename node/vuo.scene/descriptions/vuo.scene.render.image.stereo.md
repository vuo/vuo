Creates an image that is a stereoscopic snapshot of a 3D scene.

Use this in conjunction with the `Make Stereo Camera` or `Make Targeted Stereo Camera` nodes.

   - `objects` — The 3D objects to place in the scene.
   - `cameraName` — The name of the camera through which the scene is viewed. The camera is selected from any in `objects`. If no matching camera is found, then a camera from `objects` is arbitrarily chosen. If there is no camera, the default camera is used. 
   - `width`, `height` — The width and height of the resulting images, in pixels.
   - `colorDepth` — The number of bits to use to represent each color channel in the output image; higher values are more precise but use more graphics storage space.  Images are typically 8 bits per channel ("8bpc"), though if you're working with image feedback, you may want to use higher precision.
   - `leftImage`, `rightImage` — The snapshot images of the scene, one for the left and right eyes. If no scene object is visible in part of the image, that part of the image will be transparent.
   - `leftDepthImage`, `rightDepthImage` — Counterparts to the snapshot images. The depth image represents the distance between the camera and objects in the scene. Objects closer to the camera are darker, objects further from the camera are lighter. 
