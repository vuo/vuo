Fires events when the user drags files from other apps into the composition's window.

Connect the `Showed Window` output of a [Render Scene](vuo-node://vuo.scene.render.window)/[Layers](vuo-node://vuo.layer.render.window)/[Image to Window](vuo-node://vuo.image.render.window) node to this node's input port.

When the user drags a file from another app, the `Drag Entered` port fires an event when the dragged file begins hovering over the window.

As the user moves the mouse, the `Drag Moved To` port fires a series of events with the current position.

If the user releases the file over the window, the `Drag Completed` port fires an event.

If the user continues dragging the file outside the window, without releasing it over the window, the `Drag Exited` port fires an event.

Use the [Get Drag Values](vuo-node://vuo.ui.get.drag) node to get the position and URLs being dragged.
