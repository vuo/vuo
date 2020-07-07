Nodes for working with the graphics windows displayed by a composition — changing their size, position, and other properties, and getting information about them.

Nodes in other node sets — [Render Image to Window](vuo-node://vuo.image.render.window2), [Render Layers to Window](vuo-node://vuo.layer.render.window2), and [Render Scene to Window](vuo-node://vuo.scene.render.window2) — can display a window in a composition. Each of those nodes has two ports that are important to the `vuo.window` node set:

   - `Set Window Description` input port — [Change Window Size](vuo-node://vuo.window.size2), [Change Window Screen](vuo-node://vuo.window.fullscreen2), and other `vuo.window` nodes can provide input values to change properties of the window.
   - `Updated Window` output port — [Get Window Dimensions](vuo-node://vuo.window.get.dimensions3) and [Get Window Screen](vuo-node://vuo.window.get.screen) can use the output value to get information about the window.

### Window description
A **window description** is a group of settings that specify the appearance of a window or the way that a user can interact with it. For example, the "title" setting specifies the text displayed in the window's title bar. The "resizable" setting decides whether the window should change size when the user drags the window's corner.

You can chain multiple `vuo.window` nodes together, connecting the `Changed Window Description` output port of one to the `Set Window Description` input port of the next, to build a window description with multiple settings.

You can feed a window description into the `Set Window Description` input port of a [Render Scene](vuo-node://vuo.scene.render.window2)/[Layers](vuo-node://vuo.layer.render.window2)/[Image to Window](vuo-node://vuo.image.render.window2) node.

### Parts of a window
A window is divided into the **content area** and the **title bar**, as illustrated below. When the window is fullscreen, the title bar is hidden. Many nodes in this node set work specifically with the content area, for example changing its size or aspect ratio. 

![Parts of a window](labeled-window.png)

### Windows and screens
The [Get Window Screen](vuo-node://vuo.window.get.screen) node outputs the current screen a window appears on, and the [Change Window Screen](vuo-node://vuo.window.fullscreen2) node moves it to another screen or toggles its fullscreen status.

You can use the [Get Screen Values](vuo-node://vuo.screen.get) node to examine the properties of a screen.
