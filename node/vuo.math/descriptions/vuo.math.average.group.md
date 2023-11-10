Calculates the average (mean) of groups of consecutive items within the list.

For example, if the input list contains 6 elements and `Items per Group` is 3, the output list will contain the mean of the first 3 values followed by the mean of the second 3 values.

If the input list does not divide evenly into `Items per Group`, the output list will include the mean of the final partial input group as if it were a full group.

If there are no input values, the node outputs an empty list.

For point types, the node averages each component of the value.

Thanks to [Martinus Magneson](https://community.vuo.org/u/MartinusMagneson) for developing the node this is based on.
