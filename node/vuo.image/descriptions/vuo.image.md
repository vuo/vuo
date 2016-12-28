These nodes are for working with 2D images. 

Many of the nodes in this node set are **image filters** — they take an input image and apply an effect to it to produce an output image. To see a list of the image filter nodes, search the Node Library for "image filter".

To use an image file within a composition, you can import it with the `Fetch Image` node. The node supports files with extension .png, .jpeg, .jpg, .gif, .bmp, .exr, .hdr, .psd, .raw, .cr2, .dng, .dcr, .nef, .raf, .mos, .kdc, .tif, .tiff, and .tga.

In **Vuo Coordinates**, (0,0) is the center of an image. An image has a width of 2, with X-coordinate -1 on the left edge and 1 on the right edge. An image's height is determined by its aspect ratio, with the Y-coordinate increasing from bottom to top.

![Vuo Coordinate System](vuo-coordinates-transparent.png)

To use a 2D image in 3D space, you can use the `Make 3D Object from Image` in the `vuo.scene` node set. 
