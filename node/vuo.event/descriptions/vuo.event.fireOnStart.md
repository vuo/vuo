Fires an event when the composition starts.

Typically you would want just one of these nodes in a composition. Multiple `Fire on Start` nodes fire separate events and are not synchronized with each other.

There is no guarantee that this node will be the first to fire within a composition. For example, a rapidly firing [Fire Periodically](vuo-node://vuo.time.firePeriodically2) node may fire first.

An alternative to this node that's more suitable for protocol compositions, since it doesn't fire an event, is [Allow First Event](vuo-node://vuo.event.allowFirst).
