Tries to find human hands in an image, including the positions of various skeletal joints.

   - `Minimum Confidence` — The threshold for detecting a hand, ranging from 0 (detect more hands, with more false positives) to 1 (detect fewer hands, with fewer false positives).
   - `Rectangles` — The location and size of all detected hands, in Vuo Coordinates, ordered from most prominent (largest) to least.  Use this if you want to know the general areas in which hands are located.
   - `Points` — The full collection of points detected on all detected hands, in no particular order.  Use this if you want to render a constellation.
   - `Line Segments` — A pair of points for each skeletal joint in each detected hand, in no particular order.  Use this if you want to render skeletons.
   - `Bodies` — Detailed information about each individual detected hand, in order from most prominent (largest) to least.  Use this if you want to get the position of a particular feature on a particular hand — for example, get the first (most prominent) hand in the list and use its index finger tip position to control a screen pointer.  Use [Get Hand Landmarks](vuo-node://vuo.image.get.hand) to get individual values.

Since finding hands is a computationally intensive operation, you'll probably want to set the Event Throttling mode of the trigger port driving this node to "Drop Events".
