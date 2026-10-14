Retrieves the positions of hand features/joints, in Vuo Coordinates, from the output of the [Find Hands in Image](vuo-node://vuo.image.findHands) node.

The thumb and finger outputs are lists where the first item is the tip, followed by the 3 joints leading back to the wrist.

If a particular feature is not detected or is below the Minimum Confidence threshold, the value is (NaN, NaN).  You can feed these values directly to (for example) [Make Points Object](vuo-node://vuo.scene.make.points), and it will ignore the NaN points.
