Scales a value so that it's always within the specified range.

This node keeps track of the minimum and maximum values it receives over time.  Then it looks at the current value relative to its minimum and maximum, and scales it to be between `Fitted Min` and `Fitted Max`.

This node is useful if you have a data source with uncertain values (such as audio loudness) and you want to constrain it.

   - `Tracking` — The sensitivity to outliers when updating the tracked range.  This port controls how `Min` and `Max` change when the current `Value` is less than `Min` or greater than `Max`.
      - 1 — `Min` or `Max` changes to the current `Value`.
      - Between 1 and 0 — `Min` or `Max` changes to a number between its old amount and the current `Value`.
      - 0 — `Min` or `Max` don't change.  This is useful if `Tracking` has first been set to greater than 0, then given some time to calibrate before setting it to 0
   - `Reset` — Changes `Min` and `Max` to the current `Value`.
   - `Fitted Value` — The scaled `Value`.  It always falls between `Fitted Min` and `Fitted Max`.
   - `Min` and `Max` — The size of the tracked range.  How closely this range matches the actual input values depends on `Tracking`.
