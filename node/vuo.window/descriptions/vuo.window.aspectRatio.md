Creates a Window Property that changes the aspect ratio of a window.

The aspect ratio is the ratio of width to height of the window's content area. Changing a window's aspect ratio causes the window to immediately resize if it doesn't already have that aspect ratio. The window remains constrained to that aspect ratio: if the user tries to resize it, then it will adjust to preserve the aspect ratio. (The `Change Window Size` node can resize the window to a different aspect ratio, but the window will revert to the aspect ratio set by this node if the user resizes it.) 

The aspect ratio can be disabled, allowing the window to move freely, by using the `Reset Window Aspect Ratio` node. 

If the window is fullscreen when it receives this window property, the window's aspect ratio doesn't change until the window becomes non-fullscreen. 
