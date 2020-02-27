Repeatedly fires events at a steady rate.

   - `Seconds` — The interval between fired events.
   - `Fired at Time` — The time elapsed since the composition started or this node was added to a running composition.

If `Seconds` changes, the node fires its next event when the updated interval of time has passed since it fired its previous event. If more than `Seconds` seconds have already elapsed, the node fires immediately.

The fastest this node will fire is every 0.001 seconds, even if `Seconds` is smaller. If `Seconds` is zero or negative, this node doesn't fire any events.

An alternative to this node that's more suitable for protocol compositions, since it doesn't fire events, is [Allow Periodic Events](vuo-node://vuo.time.allowPeriodic).
