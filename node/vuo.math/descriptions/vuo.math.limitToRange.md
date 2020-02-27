Restricts a value to fall within a given range. 

   - `Value` — The value to restrict. 
   - `Minimum` — The lowest value in the range. 
   - `Maximum` — The highest value in the range. 
   - `Wrap Mode` — The way to adjust the value when it falls outside the range.
      - "Wrap" makes the value wrap around using modular (clock) arithmetic.
      - "Saturate" makes the value stay at the maximum when it would go over, and stay at the minimum when it would go under.

For the Integer type, the minimum and maximum are inclusive (a closed interval) — the output value can become exactly the minimum or maximum.  But for the Real type, when wrapping, the minimum is inclusive and maximum is exclusive (a half-closed interval) — the output value can become exactly the minimum, but cannot become exactly the maximum.
