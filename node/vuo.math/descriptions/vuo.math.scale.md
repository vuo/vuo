Converts a number or point from one unit of measure to another.

   - `Value` — The number to scale.
   - `Start`, `End` — Two numbers in the same units as `Value`.
   - `Scaled Start`, `Scaled End` — Two corresponding numbers in the same units as `Scaled Value`.

Examples: 

   - To convert 2 hours to minutes, set `Value` to 2, `Start` to 0, `End` to 1, `Scaled Start` to 0, and `Scaled End` to 60. The output is 120 (minutes).
   - To convert a value that falls between 0 and 100 to a value that falls between 20 and 30, set `Start` to 0, `End` to 100, `Scaled Start` to 20, and `Scaled End` to 30.
