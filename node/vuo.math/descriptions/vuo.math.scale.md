Converts a number from one unit of measure to another.

   - `value` — The number to scale.
   - `start`, `end` — Two numbers in the same units as `value`.
   - `scaledStart`, `scaledEnd` — Two corresponding numbers in the same units as `scaledValue`.

Examples: 

   - To convert 2 hours to minutes, set `value` to 2, `start` to 0, `end` to 1, `scaledStart` to 0, and `scaledEnd` to 60. The output is 120 (minutes).
   - To convert a value that falls between 0 and 100 to a value that falls between 20 and 30, set `start` to 0, `end` to 100, `scaledStart` to 20, and `scaledEnd` to 30.
