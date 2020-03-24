Fires an event each time the [Xbox One Kinect](https://en.wikipedia.org/wiki/Kinect#Kinect_for_Xbox_One_%282013%29) provides a new image.

This node can be used to display body movements captured by a Kinect in your composition.

   - `Received Color Image` — Fires an event each time the Kinect provides an image from its color video camera.
   - `Received Depth Image` — Fires an event each time the Kinect computes a depth image. This is a grayscale image in which closer objects are colored lighter, farther objects are colored darker, and objects with unknown distance are transparent.
   - `Received Infrared Image` — Fires an event each time the Kinect provides an image from its infrared video camera.

This node supports Kinect v2 (Kinect for Xbox One, Kinect v2 for Windows).  (See [Receive Kinect v1 Images](vuo-node://vuo.kinect.receive2) for Xbox 360 Kinect.)

   - The Xbox One Kinect must be plugged in to a [USB 3 port](https://support.apple.com/en-us/HT201163) on a "SuperSpeed Bus".  Neither the USB 3 "High-Speed Bus" nor USB 2 ports (common on Macs made in 2015 and earlier) work with the Xbox One Kinect.
   - Under System Information > USB, there must be both a "NuiSensor Adaptor" device and a "Xbox NUI Sensor" device.  If you don't see both, your Xbox One Kinect isn't connected properly or is broken.

When working with the depth and infrared images, you can change the image's gamma curve using [Adjust Image Colors](vuo-node://vuo.image.color.adjust) — gamma values less than 1 will make distant objects brighter.  You can add false color using [Map Image Colors](vuo-node://vuo.image.color.map).

For skeletal tracking with Kinect v1 and v2, use the [Filter Skeleton](vuo-node://vuo.osc.skeleton.basic) node with [Delicode NI mate 2](https://ni-mate.com/).
