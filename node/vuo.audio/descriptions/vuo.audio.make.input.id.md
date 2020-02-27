Finds an audio input device that is connected to the computer running the composition.

This node is useful when you know the ID of the audio input device that you want. For example, you can use it in place of [Specify Audio Input by Name](vuo-node://vuo.audio.make.input.name) if there are multiple devices with the same name and you know the ID of the one you want. If you don't know the ID, then you can instead use [Specify Audio Input by Name](vuo-node://vuo.audio.make.input.name), [Specify Audio Input by Model](vuo-node://vuo.audio.make.input.model), or [List Audio Devices](vuo-node://vuo.audio.listDevices2).

   - `ID` — The device's ID, which is 0 or greater. 
