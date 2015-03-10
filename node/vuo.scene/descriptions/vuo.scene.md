These nodes are for working with 3D scenes. 

A **scene** (scenegraph) contains a set of 3D objects. Each **object** has a set of vertices (mesh) that defines its shape. You can control how the object looks when rendered (such as color and texture) by changing its **shader**. You can control the position, angle, and size of the object by changing its **transform**. You can control the point of view on the scene by adding a **camera**. 

In **Vuo coordinates**, (0,0,0) is the center of the scene. The **default camera** is at (0,0,1). When viewed through this camera, the scene has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. The scene's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. The z-coordinate increases from back to front. Objects are visible if their distance from the camera along the z-axis is between 0.1 and 10. 

![Vuo Coordinate System](vuo-coordinates-transparent.png)

If you have a 3D object file that you'd like to import into Vuo, you can use the `Get Scene` node. 

If you'd like to create a scene within Vuo, you can start with the `Make 3D Object` node or one of the shortcut nodes, such as `Make Cube` or `Make 3D Object from Image`. 

To display a scene, you can use the `Render Scene to Window` node. 
