Creates ambient light that can be added to a 3D scene.

Ambient light affects all 3D objects having shaders that respond to lighting — regardless of the direction the object is facing, the object becomes brighter.

Typically indoor scenes and scenes on Earth's surface should have some ambient light (with a small `brightness`, around 0.1 or 0.2), to approximate light that is reflected off other objects in the scene (which isn't currently practical to compute in realtime).  However, an outer space scene, for example, typically should not have ambient light, since there are fewer objects to reflect the light, and no atmosphere to scatter the light.

If multiple `Make Ambient Light` nodes are part of the same scene, each makes the scene brighter.
