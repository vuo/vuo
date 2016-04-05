Allows one event through, and blocks all events after that.

   - `Event` — The first event received by this port passes through the output port. Events after that are blocked.
   - `Reset` — Resets the node, so that the next event into the `Event` port will be allowed through, and events after that will be blocked.

If the same event hits both the `Event` port and the `Reset` port, then that event is allowed through, and subsequent events that only hit the `Event` port are blocked.
