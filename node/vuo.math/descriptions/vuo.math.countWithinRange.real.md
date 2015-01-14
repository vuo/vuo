Keeps track of a count that can be incremented and decremented and always stays within a given range.

Before this node has received any events that change the count, the count is 0.

   - `increment` — When this port receives an event, the count is incremented by this port's value.
   - `decrement` — When this port receives an event, the count is decremented by this port's value.
   - `setCount` — When this port receives an event, the count is changed to this port's value. 
   - `minimum` — The minimum value that the count may have.
   - `maximum` — The maximum value that the count may have.
   - `wrapMode` — The way to adjust the count when it falls outside the range.
      - "Wrap" makes the count wrap around using modular (clock) arithmetic.
      - "Saturate" makes the count stay at the maximum when it would go over, and stay at the minimum when it would go under.

If an event reaches both the `setCount` port and the `increment` or `decrement` port, the count is set, and the increment or decrement is ignored. After the count is set, wrapping is applied. 
