Outputs only a rectangular part of an image. 

The rectangular part is defined by the `Top Left` point, `Width`, and `Height`, all in pixels. The top left corner of the image is at (0, 0). 

The rectangle is limited to the input image's boundaries. If the rectangle is larger than the input image, then the output image will be the same as the input image. 

As an example, with `Top Left` = (50, 38), `Width` = 100, `Height` = 75, the cropped image would be the middle of an original 200x150 pixel image. 

![](mountains.png)

![](crop.png)
