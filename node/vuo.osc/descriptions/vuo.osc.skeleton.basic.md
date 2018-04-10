Collects OSC messages from [_Delicode NI mate 2_](https://ni-mate.com/) to form points representing a human skeleton.

This node stores data from the individual OSC messages it receives for a single skeleton (e.g., `Hand`, `Left_Shoulder`), and blocks those events.  When it receives the final OSC message for the frame (`/NI_mate_sync`), it outputs that event through all its output ports.  Each output port is a 3D point, in Vuo Coordinates, representing the current position of a joint.

In _Delicode NI mate 2_, ensure that **Preferences > General > Sync frames** is checked, skeleton tracking is enabled, and **Skeleton tracking > Joint OSC mapping > Format** is set to **Basic**.

If you're tracking a single user, set `User` to 0.  This node will then expect OSC addresses without a numeric suffix, such as `Right_Hand` or `Left_Shoulder`.

If you're tracking multiple users, set `User` to the user number you'd like this node to track.  This node will then expect OSC addresses suffixed with the user number, such as `Right_Hand_1` or `Left_Shoulder_1`.  Use a separate node to collect data for each tracked user.
