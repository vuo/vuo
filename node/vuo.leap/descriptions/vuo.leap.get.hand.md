Gives information about a hand detected within a frame from a Leap Motion device.

   - `id` - A unique ID for this hand. 
   - `direction` - A unit vector pointing from `palmPosition` toward the fingers.
   - `palmNormal` - A unit vector pointing outward from (perpendicular to) the front surface of the palm.
   - `palmPosition` - The center of the palm, in Vuo coordinates.
   - `palmVelocity` - The rate of change of `palmPosition`, in Vuo coordinates per second.
   - `sphereRadius` - The radius of a ball that could be held by this hand, in Vuo coordinates.
   - `sphereCenter` - The center of a ball that could be held by this hand, in Vuo coordinates.
   - `pointables` - A list of all pointables (fingers and tools) attached to this hand.

`sphereRadius` and `sphereCenter` are based on a sphere calculated to fit the curvature of the palm and finger. It approximates the largest ball that the hand could hold in its current position.
