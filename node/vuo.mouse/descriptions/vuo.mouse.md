These nodes are for capturing input from a mouse or touchpad. 

The composition must have at least one window for these nodes to output any mouse information. 

These nodes can track mouse movements, button actions, and scrolling. For mouse movements and button actions, they can output mouse positions and deltas (changes in position) either relative to the screen or relative to a graphics window. 

If you leave the `window` input port of the mouse node unconnected, then it will output mouse positions and deltas in screen coordinates. The top left corner of the screen is (0,0). The bottom right corner of the screen matches the resolution of the display. For example, a 1440x900 display has bottom right coordinate (1400,900). 

If you connect the `window` output port of a graphics window node (such as `Render Scene to Window`) to the `window` input port of one of the mouse nodes, then the mouse node will only output mouse movements and button actions when that window is the active (frontmost) window. It will output mouse positions and deltas in Vuo coordinates, relative to the graphics area of the window. 

![Vuo Coordinate System](vuo-coordinates-transparent.png)
