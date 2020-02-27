Tries to find human faces in an image.

The locations of any detected faces are output via the `Rectangles` port, in Vuo Coordinates.

   - `Minimum Confidence` — The threshold for detecting a face, ranging from 0 (detect more faces, with more false positives) to 1 (detect fewer faces, with fewer false positives).
   - `Quality` — Controls the tradeoff between speed and thoroughness, ranging from 0 (fast, but detects only faces occupying a large portion of the image) to 1 (slower, but detects smaller faces).

Since this is a computationally intensive operation, you'll probably want to set the Event Throttling mode of the trigger port driving this node to "Drop Events".
