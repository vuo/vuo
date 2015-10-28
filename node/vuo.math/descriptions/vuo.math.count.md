Keeps track of a count that can be incremented and decremented.

When the composition starts or this node is added to a running composition, the count is the value of the `Set Count` input port. 

   - `Increment` — When this port receives an event, the count is incremented by this port's value.
   - `Decrement` — When this port receives an event, the count is decremented by this port's value.
   - `Set Count` — When this port receives an event, the count is changed to this port's value. 
   
If an event reaches both the `Set Count` port and the `Increment` or `Decrement` port, the count is set, and the increment or decrement is ignored. 
