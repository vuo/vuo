Violently shatters the object.

This node takes each trianglar face in the object's mesh, and moves it away from the explosion's center point.

   - `Time` — The time at which to calculate the explosion.
   - `Translation Amount`, `Rotation Amount` — The amount the object's faces should move or spin as a result of the explosion.  If 0, the object's faces are not moved or spun by the explosion.  Rotation affects triangular meshes, but not point or line meshes.
   - `Chaos` — The amount of variance in the explosion's effect on the object's faces.  If 0, the object's faces move and spin uniformly.  If greater than 0, the object's faces move and spin at different rates.
   - `Center` — The position of the explosion's epicenter.
   - `Range` — The distance from `Center` that the object's faces are affected by the explosion.  If `Chaos` is greater than 0, the edge becomes uneven between affected and unaffected faces.
   - `Gravity` — The speed (acceleration) and direction to pull the object's faces (in addition to the explosion force).
