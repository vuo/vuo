Selects among a list of options using a normalized real number (0 to 1).

For example, if the output port is set to the `Text Case` data type, which has 4 values, the node will behave like this:

Input          | Output
-------------- | ---------------------------------------
Less than 0.25 | Lowercase all
0.25 to 0.5    | Uppercase all
0.5 to 0.75    | Uppercase first letter of each word
0.75 or higher | Uppercase first letter of each sentence
