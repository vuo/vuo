Outputs part of the input data.

   - `Data` — The original, whole data.
   - `Start Byte` — The position of the first byte of `Data` to output. 1 is the first byte, 2 is the second byte, etc.
   - `Byte Count` — The number of bytes to output.

If `Start Byte` is less than 1 or `Byte Count` goes past the end of `Data`, then the portion of `Data` that falls within the range is output. For example, if `Byte Count` is too large, then the part of `Data` from `Start Byte` to the end is output.
