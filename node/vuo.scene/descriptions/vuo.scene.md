These nodes are for working with 3D scenes. 

A **scene** (scenegraph) contains a set of 3D objects. Each **object** has a set of vertices (mesh) that defines its shape. You can control how the object looks when rendered (such as color and texture) by changing its **shader**. You can control the position, angle, and size of the object by changing its **transform**. You can control the point of view on the scene by adding a **camera**. 

To load a 3D model from a file, you can use the `Fetch Scene` node. It supports files with extension .3dgs, .3ds, .ac3d, .b3d, .blend, .cob, .dae, .dxf, .fbx, .hmp, .irr, .irrmesh, .lwo, .lxo, .m3, .md2, .md3, .md5, .mdc, .mdl, .ms3d, .ndo, .nff, .obj, .off, .pk3, .ply, .q3d, .q3s, .raw, .scn, .smd, .stl, .ter, and .xml.

To make graphics rendering more efficient, by default Vuo only renders the front/exterior side of a 3D object. For example, with `Make Sphere Vertices`, Vuo shows the sphere when viewed from the outside — but if you move the camera inside the sphere, it disappears. You can make the back/interior of the object visible with the `Show Back of 3D Object` node. 

In **Vuo Coordinates**, (0,0,0) is the center of the scene. The **default camera** is at (0,0,1). When viewed through this camera, the scene has a width of 2, with X-coordinate -1 on the left edge and 1 on the right edge. The scene's height is determined by its aspect ratio, with the Y-coordinate increasing from bottom to top. The Z-coordinate increases from back to front. Objects are visible if their distance from the camera along the Z-axis is between 0.1 and 10. 

![Vuo Coordinate System](vuo-coordinates-transparent.png)

If you'd like to create a scene within Vuo, you can start with the `Make 3D Object` node or one of the shortcut nodes, such as `Make Cube` or `Make 3D Object from Image`. 

To display a scene, you can use the `Render Scene to Window` node. 
