These nodes are for tracking hands, fingers, and tools with a Leap Motion device. 

Data from a Leap Motion device is grouped into **frames**. Each frame contains information about **hands** and **pointables** (fingers and tools) detected at one moment in time. 

Leap Motion considers a pointable to be a **tool** if it's thinner, straighter, and longer than Leap Motion's parameters for a **finger**. 

Each hand or pointable has a unique **ID** that can be used to track it across consecutive frames in which it's detected. If the hand or pointable is lost and re-detected, its ID changes. 

Leap Motion provides a **touch plane** that can be used for interaction. Derived from the current hand and finger position within a frame, the touch plane is an imaginary surface that pointables can interact with. If a pointable is near the touch plane, then it can be in one of two **touch zones**. If the pointable is entirely on the hand's side of the touch plane, then it's in the "hovering" touch zone. If the pointable is touching or passing through the touch plane, then it's in the "touching" touch zone. 

These nodes use **Vuo Coordinates** based on Leap Motion's **interaction box**, which is a rectangular box within the Leap Motion device's field of view. Most hand motions are expected to take place within the interaction box, although some may fall outside of it and still be detected by the Leap Motion device. The interaction box's center is at Vuo Coordinate (0,0,0). The interaction box's left edge is at Vuo X-coordinate -1, and its right edge is at Vuo X-coordinate 1. The Vuo Y-coordinates increase from bottom to top of the interaction box. The Vuo Z-coordinates increase from back to front of the interaction box. 

The `Receive Leap Frame` node is your starting point for working with a Leap Motion device. 

The `Get Frame Values` node lets you get the information from a frame. From there, the `Get Hand Values` and `Get Pointable Values` nodes let you get further information about hands and pointables. 

### Troubleshooting

If `Receive Leap Frame` is not firing events:

   - Confirm that the Leap device is plugged in.
   - Check that your Leap service is running (the Leap service bar in the top right toolbar should be green).
   - Make sure your Leap Motion software is up to date. Vuo requires version 2.0.5 or later. You can check your version by going to Leap service bar > Settings... > About. You can download the most recent version from the [Leap Motion website](https://www.leapmotion.com).
