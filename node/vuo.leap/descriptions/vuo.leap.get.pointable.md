Gives information about a pointable (finger or tool) detected within a frame from a Leap Motion device.

   - `id` - A unique ID for this pointable. 
   - `type` - The type of pointable (finger or tool). 
   - `length` - The length of this pointable, from the hand to the tip (or whatever portion is visible), in Vuo coordinates. If Leap Motion can't determine the length, this outputs 0.
   - `width` - The average width of this pointable (or whatever portion is visible), in Vuo coordinates. If Leap Motion can't determine the width, this outputs 0.
   - `direction` - A unit vector pointing in the same direction as this pointable's tip.
   - `tipPosition` - The position of this pointable's tip, in Vuo coordinates.
   - `tipVelocity` - The rate of change of `tipPosition`, in Vuo coordinates per second.
