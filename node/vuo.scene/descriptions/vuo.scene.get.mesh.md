Gives information about a 3D object's mesh structure.

   - `Positions` — The vertices that define the shape of the mesh.
   - `Normals` — For each position, the vector perpendicular to the mesh surface at that position.

This node outputs the positions and normals of the 3D object itself and all of its child objects. The objects' transforms are applied to the positions and normals.

For 3D objects that don't have a mesh (cameras, lights), this node outputs empty lists.

Thanks to [Karl Henkel](https://community.vuo.org/u/khenkel) for developing the node this is based on.
