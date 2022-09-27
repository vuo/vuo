Nodes for listing and getting information from screens (monitors or displays).

### Screen coordinates
The dimensions of a screen are measured in **points**. For non-Retina displays, points are equivalent to pixels. For Retina displays, each linear point is 2 pixels.  See the [Image](vuo-nodeset://vuo.image) node set documentation for more information about points and pixels.

Positions on a screen are given in **screen coordinates**. If you have a single screen, its top left corner is at the point (0,0), and its bottom right corner matches the resolution (width and height) of the screen in points. For example, 1440x900 display would have bottom right coordinate (1440,900).  If multiple screens are attached, they occupy a continuous coordinate space based on the layout in System Settings > Displays > Arrangement.  The top-left of the primary screen — the screen with the white menu bar rectangle in System Settings — is always (0,0).  You can find the top-left coordinates of the other screens using the [Get Screen Values](vuo-node://vuo.screen.get) node.

### Getting information about screens
The [List Screens](vuo-node://vuo.screen.list2) node outputs a list of the connected screens, including the device name, position, size, and DPI.  You can use the [Get Screen Values](vuo-node://vuo.screen.get) node to examine the properties of a screen.

The [Get Window Screen](vuo-node://vuo.window.get.screen) node outputs the current screen a window appears on.

On macOS, you can look up information about the connected screens in System Settings > Displays, or by opening the System Information app and selecting Graphics/Displays.
