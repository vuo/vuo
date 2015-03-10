Outputs only a rectangular part of an image. 

The rectangular part is defined by the `topLeft` point, `width`, and `height`, all in pixels. The top left corner of the image is at (0, 0). 

The rectangle is limited to the input image's boundaries. If the rectangle is larger than the input image, then the output image will be the same as the input image. 

As an example, with `topLeft` = (50, 38), `width` = 100, `height` = 75, the cropped image would be the middle of an original 200x150 pixel image. 

![](mountains.png)

![](crop.png)
