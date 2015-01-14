Reads an integer number from text.

If the text consists of an integer (such as *-5* or *123*), then this node outputs that integer. The integer may be in scientific notation (such as *1e7* or *20e+3*).

If the text consists of an integer followed by other characters (such as *-5a* or *123.456*), then this node outputs the integer (such as -5 or 123).

If the text doesn't start with an integer (such as *x* or *.5*), then this node outputs 0.

If the text consists of an integer that is too large (positive or negative) to represent in 64 bits, then this node outputs the closest integer that can be represented.
