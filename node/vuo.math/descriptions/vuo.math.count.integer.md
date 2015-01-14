Keeps track of a count that can be incremented and decremented.

Before this node has received any events that change the count, the count is 0.

   - `increment` — When this port receives an event, the count is incremented by this port's value.
   - `decrement` — When this port receives an event, the count is decremented by this port's value.
   - `setCount` — When this port receives an event, the count is changed to this port's value. 
   
If an event reaches both the `setCount` port and the `increment` or `decrement` port, the count is set, and the increment or decrement is ignored. 
