Creates a Window Property that removes a window's aspect ratio constraint.

This node only affects windows that have previously been modified with the [Lock Window Aspect Ratio](vuo-node://vuo.window.aspectRatio) node.

When a window created with [Render Scene to Window](vuo-node://vuo.scene.render.window) or [Render Layers to Window](vuo-node://vuo.layer.render.window) receives this property, you will be able to freely resize it.

When a window created with [Render Image to Window](vuo-node://vuo.image.render.window) receives this property, it will resume being constrained to the aspect ratio of the images it receives.

If the window is fullscreen when it receives this window property, the window's aspect ratio doesn't change until the window becomes non-fullscreen. 
