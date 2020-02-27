Removes a window's aspect ratio constraint.

This node only affects windows that have previously been modified with the [Lock Window Aspect Ratio](vuo-node://vuo.window.aspectRatio2) node.

When this window description is applied to a [Render Scene to Window](vuo-node://vuo.scene.render.window2) or [Render Layers to Window](vuo-node://vuo.layer.render.window2) node, you will be able to freely resize the window.

When this window description is applied to a [Render Image to Window](vuo-node://vuo.image.render.window2) node, the window will resume being constrained to the aspect ratio of the images it receives.

If the window is fullscreen when it receives this window setting, the window's aspect ratio doesn't change until the window becomes non-fullscreen.
