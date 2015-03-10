Outputs only a rectangular part of an image. 

The rectangular part is defined by the `center` point, `width`, and `height`, all in Vuo coordinates. 

The rectangle is limited to the input image's boundaries. If the rectangle is larger than the input image, then the output image will be the same as the input image. 

As an example, with `center` = (0, 0), `width` = 1, `height` = 1, the cropped image would be the middle of the original image. 

![](mountains.png)

![](crop.png)
