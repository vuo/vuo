Calculates the average (mean) of the values received by this node so far.

The 1st item in `Averages` is the average of the current 1st item in `Values` and the 1st item in previous `Values` lists; the 2nd item in `Averages` corresponds to the 2nd item in `Values`; and so on.

   - `Values` — When this port receives an event, each list item is averaged in with the previous values of that list item.
   - `Lookback` — The number of previous values to include in the average.
      - If 10, for example, then the node calculates a rolling average based on the current `Values` list and the 10 previous `Values` lists.
      - If 0, the average is just `Values`.
      - If `Auto`, the average includes all previous `Values` in its history.
   - `Reset` — When this port receives an event, the node's history is cleared. It forgets about all previous `Values` and begins calculating the average anew with the next `Values`.

If an event hits both `Values` and `Reset`, then the history is reset before the average is recalculated. In other words, the node outputs `Values`.

If you increase `Lookback` from *A* to *B* while the composition is running, then the next event to `Values` will calculate the average based on `Values` and the previous *A* values in its history.

For point types, this node averages each component (X, Y, Z, W) of the points separately.

Thanks to [Karl Henkel](https://vuo.org/user/32) for developing the node this is based on.
