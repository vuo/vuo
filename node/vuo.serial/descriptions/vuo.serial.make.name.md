Selects a serial device by matching its name.

Connect this node to [Send Serial Data](vuo-node://vuo.serial.send) or [Receive Serial Data](vuo-node://vuo.serial.receive) to dynamically choose a serial device to use.

   - `Name` — All or part of the device’s name (the device's USB manufacturer, model, and serial number). If more than one device matches the given name, the one with the lowest serial number is chosen.
