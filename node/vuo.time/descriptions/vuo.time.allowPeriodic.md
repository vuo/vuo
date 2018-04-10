Periodically lets a single `Time` event through, so that events are output at a steady rate.

This node allows the first `Time` event through that is greater than or equal to `Period`. After that, it allows the first time event through that is greater than or equal to the next multiple of `Period`, and so on.

When `Periodic Time` outputs an event from `Time`, it also outputs the data from `Time`.

The `Time` port should receive a stream of events with increasing time values, such as from the `Requested Frame` port of a node that displays a graphics window or the `time` published input port of a protocol composition.

Successive `Time` port values should increase by an amount smaller than `Period`. The smaller the `Time` increments, the less variance in the time between output events.

If `Period` is zero or negative, this node doesn't output any events.

An alternative to this node that fires its own events is `Fire Periodically`.
