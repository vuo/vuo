These nodes are for working with 2D images. 

Many of the nodes in this node set are **image filters** — they take an input image and apply an effect to it to produce an output image. To see a list of the image filter nodes, search the Node Library for "image filter".

To use a 2D image in 3D space, you can use the `Make 3D Object from Image` in the `vuo.scene` node set.

### Pixels and channels

An image consists of a grid of **[pixels](https://simple.wikipedia.org/wiki/Pixel)**. Each pixel is represented as a combination of colors (often red, green, and blue). The image can be separated by these color components into **<a href="https://en.wikipedia.org/wiki/Channel_(digital_image)"/>channels</a>**. In addition to color channels, some images have an **alpha channel** that represents the image's opacity. An image with an alpha channel may have transparent or semi-transparent regions, whereas an image without an alpha channel is always fully opaque. The popover for an image port lists the image's channels, along with other information about the image's [format](https://www.opengl.org/sdk/docs/man/html/glTexImage2D.xhtml) that may be useful if you're writing GLSL shaders.

### GPU acceleration

Most of the nodes in this node set do their image processing on the GPU for faster performance. This includes nodes that filter images and nodes that generate images.

A few nodes do their work entirely on the CPU. This includes nodes that deal with image files (such as `Fetch Image` and `Save Image`), nodes that deal with image metadata (such as `Get Image Size`), the `Sample Color from Image` node, the `Are Equal` node, and the `Find Barcode` node.

Image data may be stored in GPU RAM only or in both GPU and CPU RAM, depending on where the image came from. Images loaded from files (such as the output of the `Fetch Image` node) are stored in both GPU and CPU RAM. Images generated on the GPU (such as the output of an image filter node) are initially stored in GPU RAM only. But if they're sent to a node that processes image data on the CPU (such as `Sample Color from Image`), they're additionally stored in CPU RAM.

### Coordinates

In **Vuo Coordinates**, (0,0) is the center of an image. An image has a width of 2, with X-coordinate -1 on the left edge and 1 on the right edge. An image's height is determined by its aspect ratio, with the Y-coordinate increasing from bottom to top.

![Vuo Coordinate System](vuo-coordinates-transparent.png)
