Calculates the average (mean) of the values received by this node so far.

   - `Value` — When this port receives an event, the port's value is averaged in with the previous values.
   - `Lookback` — The number of previous values to include in the average.
      - If 10, for example, then the node calculates a rolling average based on the current `Value` and the 10 previous `Value`s.
      - If 0, the average is just `Value`.
      - If `Auto`, the average includes all previous `Value`s in its history.
   - `Reset` — When this port receives an event, the node's history is cleared. It forgets about all previous `Value`s and begins calculating the average anew with the next `Value`.

If an event hits both `Value` and `Reset`, then the history is reset before the average is recalculated. In other words, the node outputs `Value`.

If you increase `Lookback` from *A* to *B* while the composition is running, then the next event to `Value` will calculate the average based on `Value` and the previous *A* values in its history.

For point types, this node averages each component (X, Y, Z, W) of the points separately.
