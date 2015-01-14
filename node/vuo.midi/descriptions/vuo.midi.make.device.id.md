Finds a MIDI device that is connected to the computer running the composition.

This node is useful when you know the ID of the MIDI device that you want. For example, you can use it in place of `Make MIDI Device from Name` if there are multiple devices with the same name and you know the ID of the one you want. If you don't know the ID, then you can instead use `Make MIDI Device from Name` or `List MIDI Devices`.

   - `id` — The device's ID, which is 0 or greater. 
   - `isInput` — If *true*, finds an input device. If *false*, finds an output device.
