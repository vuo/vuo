Keeps track of a count that can be incremented and decremented.

When the composition starts or this node is added to a running composition, the count is the value of the `setCount` input port. 

   - `increment` — When this port receives an event, the count is incremented by this port's value.
   - `decrement` — When this port receives an event, the count is decremented by this port's value.
   - `setCount` — When this port receives an event, the count is changed to this port's value. 
   
If an event reaches both the `setCount` port and the `increment` or `decrement` port, the count is set, and the increment or decrement is ignored. 
