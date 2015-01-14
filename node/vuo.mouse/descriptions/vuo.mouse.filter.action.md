Only lets a mouse press, release, or click pass through if it matches all conditions.

   - `action` — An event into this port is blocked unless the port's value meets all of the criteria specified by the other input ports.
   - `includeLeftButton`, `includeMiddleButton`, `includeRightButton` – If *true*, accepts actions that used the left, middle, or right mouse button. At least one of these ports must be *true* for this node to accept any mouse button actions.
   - `includePress`, `includeRelease`, `includeSingleClick`, `includeDoubleClick`, `includeTripleClick` — If *true*, accepts presses, releases, single-clicks, double-clicks, or triple-clicks. At least one of these ports must be *true* for this node to accept any mouse button actions.
