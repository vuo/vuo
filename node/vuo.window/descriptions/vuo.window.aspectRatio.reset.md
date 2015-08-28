Creates a Window Property that resets the aspect ratio of a window.

This node only affects windows that have previously been modified with the `Change Window Aspect Ratio` node.

When a window created with `Render Scene to Window` or `Render Layers to Window` receives this property, you will be able to freely resize it.

When a window created with `Render Image to Window` receives this property, it will resume being constrained to the aspect ratio of the images it receives.

If the window is fullscreen when it receives this window property, the window's aspect ratio doesn't change until the window becomes non-fullscreen. 
