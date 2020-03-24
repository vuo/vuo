Converts UTF-8 encoded data to text.

The data doesn't need to have a trailing NULL byte.  If the data includes a NULL byte, conversion stops just before it.

If the data isn't valid UTF-8, outputs empty text.
