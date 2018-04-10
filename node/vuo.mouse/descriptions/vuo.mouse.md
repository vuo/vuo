These nodes are for capturing input from a mouse or touchpad. 

If you leave the `Window` input port of a mouse node unconnected, then it will output mouse positions in screen coordinates. With a single screen, the top left corner is at position (0,0), and the bottom right corner matches the resolution (width and height) of the screen in points. For example, a 1440x900 display would have bottom right position (1440,900).

If you connect the `Showed Window` output port of a graphics window node (such as `Render Scene to Window`) to the `Window` input port of a mouse node, then the mouse node will output mouse positions in Vuo Coordinates, relative to the graphics area of the window.

If the composition doesn't have any windows, then mouse movements can be detected, but button presses and scrolling can't.

![Vuo Coordinate System](vuo-coordinates-transparent.png)
