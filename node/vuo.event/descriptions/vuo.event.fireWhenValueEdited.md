Fires an event when `Value` is changed in the Vuo editor.

This node is useful for quickly previewing a range of values while building a composition.  Connect the `Value Edited` output port, run the composition, then double-click on the `Value` port and drag the slider.

By default this node outputs values between 0 (when the slider is on the left side) and 1 (right side).  By changing `Scale`, you can specify the range of output values.

When the composition is running outside of the Vuo editor (e.g., when exported to an app or plugin), this node has no effect — it doesn't fire any events.
