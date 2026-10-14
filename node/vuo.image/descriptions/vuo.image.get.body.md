Retrieves the positions of skeletal features/joints, in Vuo Coordinates, from the output of the [Find Bodies in Image](vuo-node://vuo.image.findBodies) node.

If a particular feature is not detected or is below the Minimum Confidence threshold, the value is (NaN, NaN).  You can feed these values directly to (for example) [Make Points Object](vuo-node://vuo.scene.make.points), and it will ignore the NaN points.
