Applies a shader to a specific material in a 3D object and all of its child objects. 

This node can be used with objects created in a 3D modeling application and imported into Vuo with the `Get Scene` node. 

This node finds each object whose material name exactly matches the `material` input port or contains part of it, and replaces its shader with the given one.  If `material` is empty, this node replaces all of the object's shaders.
