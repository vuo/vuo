Scales a value so that it's always within the specified range.

This node keeps track of the minimum and maximum values it receives over time.  Then it looks at the current value relative to its minimum and maximum, and scales it to be between `Fitted Minimum` and `Fitted Maximum`.

This node is useful if you have a data source with uncertain values (such as audio loudness) and you want to constrain it.

   - `Tracking` — The sensitivity to outliers when updating the tracked range.  This port controls how `Minimum` and `Maximum` change when the current `Value` is less than `Minimum` or greater than `Maximum`.
      - 1 — `Minimum` or `Maximum` changes to the current `Value`.
      - Between 1 and 0 — `Minimum` or `Maximum` changes to a number between its old amount and the current `Value`.
      - 0 — `Minimum` or `Maximum` don't change.  This is useful if `Tracking` has first been set to greater than 0, then given some time to calibrate before setting it to 0
   - `Reset` — Changes `Minimum` and `Maximum` to the current `Value`.
   - `Fitted Value` — The scaled `Value`.  It always falls between `Fitted Minimum` and `Fitted Maximum`.
   - `Minimum` and `Maximum` — The size of the tracked range.  How closely this range matches the actual input values depends on `Tracking`.
