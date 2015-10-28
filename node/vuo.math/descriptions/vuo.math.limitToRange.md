Restricts a value to fall within a given range. 

   - `Value` — The value to restrict. 
   - `Minimum` — The lowest value in the range. 
   - `Maximum` — The highest value in the range. 
   - `Wrap Mode` — The way to adjust the value when it falls outside the range.
      - "Wrap" makes the value wrap around using modular (clock) arithmetic.
      - "Saturate" makes the value stay at the maximum when it would go over, and stay at the minimum when it would go under.
