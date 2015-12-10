Repeatedly fires events at a steady rate.

When the composition starts or this node is added to a composition, it begins firing every `Seconds` seconds.

When this node receives an event, the timer resets. The next time it fires will be `Seconds` seconds later.

The fastest this node will fire is every thousandth of a second, even if `Seconds` is smaller than that. If `Seconds` is zero or negative, this node doesn't fire any events.
