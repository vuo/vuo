These nodes are for working with the windows displayed by a composition. 

Nodes in other node sets — `Render Image to Window`, `Render Layers to Window`, and `Render Scene to Window` — can display a window in a composition. Each of those nodes has a `showedWindow` output port that can be used by the `vuo.window` node set to get information about a window, and a `windowProperties` input port that can be used by the `vuo.window` node set to change properties of the window. 

A **Window Property** affects the appearance of the window or the way that a user can interact with it. For example, the "title" Window Property sets the text displayed in the window's title bar. The "resizable" Window Property decides whether the window should change size when the user drags the window's corner. 

A window is divided into the **content area** and the **title bar**, as illustrated below. When the window is fullscreen, the title bar is hidden. Many nodes in this node set work specifically with the content area, for example changing its size or aspect ratio. 

![Parts of a window](labeled-window.png)
