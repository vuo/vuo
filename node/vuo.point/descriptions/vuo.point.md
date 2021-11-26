Nodes for doing calculations with 2D, 3D, and 4D points and vectors.

See also the [vuo.math](vuo-nodeset://vuo.math) node set, which has several nodes that work with points.

## Replicating or animating an object along a path
Some of the nodes in this node set output a list of points:

   - [Make Points along Cube Edges](vuo-node://vuo.point.make.cube)
   - [Make Points along Line](vuo-node://vuo.point.make.curve)
   - [Make Points along Oval](vuo-node://vuo.point.make.oval)
   - [Make Points along Spline](vuo-node://vuo.point.make.spline)
   - [Make Points in 3D Grid](vuo-node://vuo.point.make.grid.3d)
   - [Make Phyllotaxis Points](vuo-node://vuo.point.make.phyllotaxis)
   - [Make Parametric Points](vuo-node://vuo.point.make.parametric)
   - [Make Parametric Grid Points](vuo-node://vuo.point.make.parametric.grid)

You can connect the output of these nodes to the [Copy Layer](vuo-node://vuo.layer.copy.trs) or [Copy 3D Object](vuo-node://vuo.scene.copy.trs) node to create a series of objects along a path.

Alternatively, you can connect the output of these nodes to the [Cycle through List](vuo-node://vuo.list.cycle2) node, then connect that to [Transform Layer](vuo-node://vuo.layer.transform.trs) or [Transform 3D Object](vuo-node://vuo.scene.transform.trs) to animate an object moving along a path.
