Stretches an image across a mesh imported from a mesh data file. 

If you drag a mesh data file from your computer onto the composition, an instance of this node will be created with the `URL` port set up.

This is commonly used for projecting onto a dome with a spherical mirror, an inexpensive alternative to traditional planetarium projection systems. Besides warping images for dome projection, it can be used for any kind of image warping — for example: keystone correction, stereo image alignment, conversion between image perspectives, and projection mapping. 

[Dome projection using a spherical mirror](http://paulbourke.net/dome/) describes the image warping technique. It provides mesh data files for warping [fisheye](http://paulbourke.net/dome/warppatch/fisheye.data) or [spherical](http://paulbourke.net/dome/warppatch/spherical.data) images for spherical mirror projection onto a dome, and for warping [cylindrical](http://paulbourke.net/dome/warppatch/cylindrical.data) or [planar](http://paulbourke.net/dome/warppatch/planar.data) images to a perspective projection. It also provides the [meshmapper](http://paulbourke.net/dome/meshmapper/) software for creating custom mesh data files to fit your projector and spherical mirror. 

   - `URL` — A file of Paul Bourke's [mesh data file (.data)](http://paulbourke.net/dataformats/meshwarp/) format.
   - `Image` — The image to warp. Its projection should match the mesh data file being used. For example, if the mesh is designed to warp a fisheye image to some other projection, then the input image should have a fisheye projection.
   - `Image Position` — The translation of the input image relative to the mesh, in the same units as the mesh data file. For some input image projections (such as spherical, cylindrical, or planar), you can pan around the image by sending changing values to this port.
   - `Image Rotation` — The rotation of the input image relative to the mesh, in degrees.
   - `Width` and `Height` — The size of the output image, in pixels.
   - `Warped Image` — The image warped by the mesh, ready to be output to a projector with a `Render Image to Window` node.

See [vuo.url](vuo-nodeset://vuo.url) for info on how Vuo handles URLs.

Thanks to [Paul Bourke](http://paulbourke.net/) for developing the image warping technique used by this node and for offering guidance for implementing it in Vuo.
