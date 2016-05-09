Changes system-wide settings for a serial device.

   - `Device` — The serial device to configure.
   - `Baud Rate` — The speed, in bits per second, at which the device should send or receive data.
   - `Data Bits` — The number of bits that make up a character of data.
   - `Parity` — Whether to send or receive an error detection bit, and how to use it.
   - `Stop Bits` — How many marked bits to send or receive after each character of data.

Arduino tutorials typically specify 9600 baud, with 8 data bits, no parity, and 1 stop bit.  You might want to change the baud rate (on both ends — in your microcontroller firmware and using this node) to increase the speed, but you probably won't need to change the other settings unless you're working with a serial device designed before the mid-1980s.
