These nodes are for tracking hands, fingers, and tools with a Leap Motion device. 

Data from a Leap Motion device is grouped into **frames**. Each frame contains information about **hands** and **pointables** (fingers and tools) detected at one moment in time. 

Leap Motion considers a pointable to be a **tool** if it's thinner, straighter, and longer than Leap Motion's parameters for a **finger**. 

Each hand or pointable has a unique **ID** that can be used to track it across consecutive frames in which it's detected. If the hand or pointable is lost and re-detected, its ID changes. 

These nodes use **Vuo coordinates** based on Leap Motion's interaction box, which is a rectangular box within the Leap Motion device's field of view. Most hand motions are expected to take place within the interaction box, although some may fall outside of it and still be detected by the Leap Motion device. The interaction box's center is at Vuo coordinate (0,0,0). The interaction box's left edge is at Vuo x-coordinate -1, and its right edge is at Vuo x-coordinate 1. The Vuo y-coordinates increase from bottom to top of the interaction box. The Vuo z-coordinates increase from back to front of the interaction box. 

The `Receive Leap Frame` node is your starting point for working with a Leap Motion device. 

The `Get Frame Values` node lets you get the information from a frame. From there, the `Get Hand Values` and `Get Pointable Values` nodes let you get further information about hands and pointables. 

The `Sort Hands by Distance` and `Sort Pointables by Distance` nodes let you find the hand or pointable closest to a point. 

The `Filter Hands by ...` and `Filter Pointables by ...` nodes let you find hands or pointables that match certain criteria. 
