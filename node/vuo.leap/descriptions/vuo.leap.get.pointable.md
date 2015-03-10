Gives information about a pointable (finger or tool) detected within a frame from a Leap Motion device.

   - `id` — A unique ID for this pointable. 
   - `type` — The type of pointable (finger or tool). 
   - `length` — The length of this pointable, from the hand to the tip (or whatever portion is visible), in Vuo coordinates. If Leap Motion can't determine the length, this outputs 0.
   - `width` — The average width of this pointable (or whatever portion is visible), in Vuo coordinates. If Leap Motion can't determine the width, this outputs 0.
   - `direction` — A unit vector pointing in the same direction as this pointable's tip.
   - `tipPosition` — The position of this pointable's tip, in Vuo coordinates.
   - `stabilizedTipPosition` — The position of this pointable's tip, smoothed and stabilized by Leap Motion, in Vuo coordinates.
   - `tipVelocity` — The rate of change of `tipPosition`, in Vuo coordinates per second.
   - `touchZone` — The touch zone this pointable is in, relative to Leap Motion's touch plane. 
   - `touchDistance` — This pointable's distance from Leap Motion's touch plane, ranging from -1 to 1. At 1, the pointable is on the boundary of the "none" and "hovering" touch zones. At 0, the pointable is on the boundary of the "hovering" and "touching" touch zones. At -1, the pointable is fully within the "touching" touch zone. 
   - `timeVisible` — The time since this pointable was detected by Leap Motion, in seconds. 
