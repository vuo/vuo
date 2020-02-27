Constrains a window to a certain aspect ratio.

The aspect ratio is the ratio of width to height of the window's content area. Locking a window's aspect ratio causes the window to immediately resize if it doesn't already have that aspect ratio. The window remains constrained to that aspect ratio: if the user tries to resize it, then it will adjust to preserve the aspect ratio. (The [Change Window Size](vuo-node://vuo.window.size2) node can resize the window to a different aspect ratio, but the window will revert to the aspect ratio set by this node if the user resizes it.)

The aspect ratio constraint can be removed, allowing the window to move freely, by using the [Unlock Window Aspect Ratio](vuo-node://vuo.window.aspectRatio.reset2) node.

If the window is fullscreen when it receives this window setting, the window's aspect ratio doesn't change until the window becomes non-fullscreen.
