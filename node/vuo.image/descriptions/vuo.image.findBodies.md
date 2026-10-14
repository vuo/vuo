Tries to find human bodies in an image, including the positions of various skeletal joints.

   - `Minimum Confidence` — The threshold for detecting a body, ranging from 0 (detect more bodies, with more false positives) to 1 (detect fewer bodies, with fewer false positives).
   - `Rectangles` — The location and size of all detected bodies, in Vuo Coordinates, ordered from most prominent (largest) to least.  Use this if you want to know the general areas in which bodies are located — for example, to blur part of an image in order to anonymize it.
   - `Points` — The full collection of points detected on all detected bodies, in no particular order.  Use this if you want to render a constellation.
   - `Line Segments` — A pair of points for each skeletal joint in each detected body, in no particular order.  Use this if you want to render skeletons.
   - `Bodies` — Detailed information about each individual detected body, in order from most prominent (largest) to least.  Use this if you want to get the position of a particular feature on a particular body — for example, get the first (most prominent) body in the list and use its wrist position to control a screen pointer.  Use [Get Body Landmarks](vuo-node://vuo.image.get.body) to get individual values.

Since finding bodies is a computationally intensive operation, you'll probably want to set the Event Throttling mode of the trigger port driving this node to "Drop Events".

To track some additional points on human bodies, see [Find Faces in Image](vuo-node://vuo.image.findFaces) and [Find Hands in Image](vuo-node://vuo.image.findHands).
