Keeps track of a count that can be incremented and decremented and always stays within a given range.

When the composition starts or this node is added to a running composition, the count is the value of the `Set Count` input port. 

   - `Increment` — When this port receives an event, the count is incremented by this port's value.
   - `Decrement` — When this port receives an event, the count is decremented by this port's value.
   - `Set Count` — When this port receives an event, the count is changed to this port's value. 
   - `Minimum` — The minimum value that the count may have.
   - `Maximum` — The maximum value that the count may have.
   - `Wrap Mode` — The way to adjust the count when it falls outside the range.
      - "Wrap" makes the count wrap around using modular (clock) arithmetic.
      - "Saturate" makes the count stay at the maximum when it would go over, and stay at the minimum when it would go under.

If an event reaches both the `Set Count` port and the `Increment` or `Decrement` port, the count is set, and the increment or decrement is ignored. After the count is set, wrapping is applied. 

For the Integer type, the minimum and maximum are inclusive (a closed interval) — when incrementing and wrapping, the output value can become exactly the maximum followed by exactly the minimum.  But for the Real type, the minimum and maximum are exclusive (a half-closed interval) — when incrementing, the output value can become exactly the maximum, but cannot become exactly the minimum.
