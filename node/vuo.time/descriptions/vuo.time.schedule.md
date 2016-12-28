Calculates the time elapsed since each output port was scheduled to begin passing along events.

This node is useful for controlling animations. You can connect the `Requested Frame` output port of a node that displays a graphics window (such as `Render Scene to Window`) to the `Time` input port of this node. Whenever the `Time` input port receives an event, each `Elapsed Time` port outputs a time relative to the start time specified for that port in the `Schedule` input list. For example, if the `Time` input port receives values that begin at 0 and increase by 1 each second and the first value in the `Schedule` input list is 3, `Elapsed Time 1` will begin outputting events after 3 seconds, and will output elapsed times of 0, 1 and 2 at corresponding input times of 3, 4, and 5 seconds. You can use these elapsed times to calculate an object's position or other parameters in an animation.

   - `Time` — A time, such as the output from the `Requested Frame` port of a graphics window node. Each time value should be greater than the previous one for events to continue being transmitted on schedule. The time may be reset to return to an earlier point within the schedule.
   - `Schedule` — A list containing the points in time at which each of the 8 `Elapsed Time` ports should begin outputting events.
   - `Duration Type` — How long the `Elapsed Time` ports should continue to output events once they have begun.
      - `Single` — Output only a single event, on the first occasion that the `Time` input reaches a value greater than or equal to the output port's scheduled time.
      - `Until Next` — Output an event each time the `Time` input port receives an event, starting on the first occasion that the `Time` input reaches a value greater than or equal to the `Elapsed Time <x>` port's scheduled time but ending when the `Elapsed Time <x+1>` port begins outputting its own events.
      - `Until Reset` — Output an event each time the `Time` input port receives an event, starting on the first occasion that the `Time` input reaches a value greater than or equal to the `Elapsed Time` port's scheduled time and ending only if the input time is reset to a value less than that output port's scheduled time.
   - `Loop` — How time repeats once it exceeds the final time value in the `Schedule` input list.

   - `Elapsed Time 1` ... `Elapsed Time 8` — The difference between the current value of `Time` and the time at which the output port was scheduled to begin passing along events.
