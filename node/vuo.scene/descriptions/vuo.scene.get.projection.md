Warps an image and mesh file for use with a spherical mirror, projector, and dome.  Commonly used as an inexpensive alternative to traditional planetarium dome projection systems.  This method was created by Paul Bourke and is described in detail [here](http://paulbourke.net/dome/warppatch/).

- meshUrl - A file path to the mesh data.
- textureDelta - Move the image position on the mesh object.  As an example, you may connect the mouse position to this port to simulate a camera looking around a 3d scene.
- width / height - The width and height of the mesh object (in VuoCoordinates).
- shader - The shader the output scene object will use.
- object - A warped mesh and scene object.