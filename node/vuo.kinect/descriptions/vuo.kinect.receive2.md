Fires an event each time the [Xbox 360 Kinect](https://en.wikipedia.org/wiki/Kinect) provides a new image.

This node can be used to display body movements captured by a Kinect in your composition.

   - `Image Type` — Whether to output RGB color images or monochrome infrared images.
   - `Received Image` — Fires an event each time the Kinect provides an image from its color or infrared video camera.
   - `Received Depth Image` — Fires an event each time the Kinect computes a depth image. This is a grayscale image in which closer objects are colored lighter, farther objects are colored darker, and objects with unknown distance are transparent.

This node supports Kinect v1 (Kinect for Xbox 360, models 1414 and 1473; Kinect for Windows, model 1517).

When working with the depth and infrared images, you can change the image's gamma curve using [Adjust Image Colors](vuo-node://vuo.image.color.adjust) — gamma values less than 1 will make distant objects brighter.  You can add false color using [Map Image Brightness to Gradient](vuo-node://vuo.image.color.map).
