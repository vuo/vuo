These nodes are for working with layers to create composite images. 

In Vuo, layers are a unique data type that are similar to images, but can be combined together as if they were stacked on top of one another. 

In **Vuo coordinates**, (0,0) is the center of the composite image. The composite image has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. The composite image's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. 

![Vuo Coordinate System](vuo-coordinates-transparent.png)

To create one layer of a composite image, you can use the `Make Layer` node. 

To combine layers into a composite image, you can use the `Render Layers to Image` or `Render Layers to Window` node. 
