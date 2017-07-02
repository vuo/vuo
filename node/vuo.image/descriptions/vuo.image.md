These nodes are for working with 2D images. 

An image consists of a grid of **[pixels](https://simple.wikipedia.org/wiki/Pixel)**. Each pixel is represented as a combination of colors (often red, green, and blue). The image can be separated by these color components into **<a href="https://en.wikipedia.org/wiki/Channel_(digital_image)"/>channels</a>**. In addition to color channels, some images have an **alpha channel** that represents the image's opacity. An image with an alpha channel may have transparent or semi-transparent regions, whereas an image without an alpha channel is always fully opaque. The popover for an image port lists the image's channels, along with other information about the image's [format](https://www.opengl.org/sdk/docs/man/html/glTexImage2D.xhtml) that may be useful if you're writing GLSL shaders.

Many of the nodes in this node set are **image filters** — they take an input image and apply an effect to it to produce an output image. To see a list of the image filter nodes, search the Node Library for "image filter".

In **Vuo Coordinates**, (0,0) is the center of an image. An image has a width of 2, with X-coordinate -1 on the left edge and 1 on the right edge. An image's height is determined by its aspect ratio, with the Y-coordinate increasing from bottom to top.

![Vuo Coordinate System](vuo-coordinates-transparent.png)

To use a 2D image in 3D space, you can use the `Make 3D Object from Image` in the `vuo.scene` node set. 
