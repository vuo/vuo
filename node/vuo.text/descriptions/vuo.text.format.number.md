Formats a real number as text.

   - `Format` — Whether to treat the number as a generic decimal number, or a percentage (input ranging from 0 to 1, representing 0% to 100%), or a currency amount (including your system's currency symbol, from your _Language & Region_ settings). Negative currency amounts are formatted within parenthesis, not with a negative number sign. 
   - `Minimum Integer Digits` — The minimum number of digits to show to the left of the decimal point. Zeroes will be prefixed if necessary.
   - `Minimum Decimal Places` — The minimum number of digits to show to the right of the decimal point. Zeroes will be appended if necessary.
   - `Maximum Decimal Places` — The maximum number of digits to show to the right of the decimal point. If fewer digits than the original number, the number is rounded. If zero, the output text omits the decimal point.
   - `Show Thousands Separator` — Whether to show a separator between each group of 3 digits left of the decimal point.

This node uses your system's _Language & Region_ settings, so:

   - the decimal point may be `.` or `,` or some other character
   - the thousands separator may be `.` or `,` or `'` or a space or some other character
