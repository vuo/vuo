Gives information about a pointable (finger or tool) detected within a frame from a Leap Motion device.

   - `ID` — A unique ID for this pointable. 
   - `Type` — The type of pointable (finger or tool). 
   - `Transform` — Transforms a forward-facing object so it is at the tip of the pointable, facing in the direction of the pointable, with the detected length and width of the pointable.
   - `Tip Velocity` — The rate of change of `Tip Position`, in Vuo Coordinates per second.
   - `Touch Zone` — The touch zone this pointable is in, relative to Leap Motion's touch plane. 
   - `Touch Distance` — This pointable's distance from Leap Motion's touch plane, ranging from -1 to 1. At 1, the pointable is on the boundary of the "none" and "hovering" touch zones. At 0, the pointable is on the boundary of the "hovering" and "touching" touch zones. At -1, the pointable is fully within the "touching" touch zone. 
   - `Is Extended` - For fingers, this outputs true if the finger is extended straight from the hand as if pointing, and false if the finger is curled inward toward the palm. For tools, this always outputs true.
   - `Time Visible` — The time since this pointable was detected by Leap Motion, in seconds.
