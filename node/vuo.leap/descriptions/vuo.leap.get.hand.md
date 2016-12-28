Gives information about a hand detected within a frame from a Leap Motion device.

   - `ID` - A unique ID for this hand. 
   - `Transform` - Transforms a forward-facing object so it is at the center of the palm, facing toward the fingers, scaled to the width of the palm.
   - `Palm Velocity` - The rate of change of `Palm Position`, in Vuo Coordinates per second.
   - `Wrist Position` — The center position of the wrist, in Vuo Coordinates.
   - `Sphere Radius` - The radius of a ball that could be held by this hand, in Vuo Coordinates.
   - `Sphere Center` - The center of a ball that could be held by this hand, in Vuo Coordinates.
   - `Pinch Amount` — The degree to which the hand is performing a pinch pose, ranging from 1 (complete pinch) to 0 (no pinch). A pinch pose is when the thumb and the closest fingertip are brought together.
   - `Grab Amount` — The degree to which the hand is performing a grab pose, ranging from 1 (complete grab) to 0 (no grab). A grab pose is when the hand closes into a fist.
   - `Is Left Hand` - True if this is a left hand, false if it's a right hand.
   - `Time Visible` — The time since this pointable was detected by Leap Motion, in seconds.
   - `Confidence` — The level of confidence that the hand and fingers have been correctly interpreted by Leap Motion, ranging from 1 (most confident) to 0 (not confident at all).
   - `Fingers` - The fingers attached to this hand, listed in arbitrary order. The list contains as many fingers as are detected by the Leap Motion.

`Sphere Radius` and `Sphere Center` are based on a sphere calculated to fit the curvature of the palm and finger. It approximates the largest ball that the hand could hold in its current position.
