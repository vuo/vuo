Reads a real number from text.

If the text consists of a real number (such as *-5*, *0.6*, or *123.456*), then this node outputs that real number. The real number may be in scientifc notation (such as *10e23* or *7.8e-9*). Note that *0.6* is treated as a real number but *.6* is not. 

If the text consists of a real number followed by other characters (such as *-5a* or *123.456.789*), then this node outputs the real number (such as -5 or 123.456).

If the text doesn't start with a real number (such as *x* or *.6*), then this node outputs 0.

If the text consists of a real number that is too large or small to represent in 64 bits, then this node outputs the closest real number that can be represented.
