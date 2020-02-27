Finds a MIDI input device that is connected to the computer running the composition.

This node is useful when you know the ID of the MIDI input device that you want. For example, you can use it in place of [Specify MIDI Input by Name](vuo-node://vuo.midi.make.input.name) if there are multiple devices with the same name and you know the ID of the one you want. If you don't know the ID, then you can instead use [Specify MIDI Input by Name](vuo-node://vuo.midi.make.input.name) or [List MIDI Devices](vuo-node://vuo.midi.listDevices2).

   - `ID` — The device's ID, which is 0 or greater. 
