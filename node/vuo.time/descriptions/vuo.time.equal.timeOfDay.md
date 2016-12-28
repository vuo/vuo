Outputs *true* if all times-of-day are within a certain distance of each other.

Ignores the date components of the Date-Times; only the time components are considered.

If there are no times, outputs *true*.

`Tolerance` and `Tolerance Unit` together specify the maximum amount by which any two times-of-day can differ to still be considered equal.  For example, `2` and `Hours` means all times-of-day must be within 2 hours of each other to be considered equal.

Times are considered equal within tolerance if they span midnight.  For example, 23:00 and 01:00 are equal within a tolerance of 3 hours.
