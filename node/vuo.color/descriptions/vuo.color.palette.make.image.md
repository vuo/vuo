Creates a color palette based on the colors in an image.

   - `Image` — The image to analyze.
   - `Colors` — How many colors to output, limited to 256.
   - `Palette` — The list of colors found in the image.

This node uses Xiaolin Wu's 1991 algorithm ("greedy orthogonal bipartition of RGB space for variance minimization") to pick a set of colors that represents the overall image.

For example, given this image:  
![](mountains.png)

This node would produce the following 4-color palette:  
![](palette-4.png)

Or the following 32-color palette:  
![](palette-32.png)

All output colors are opaque; transparent colors in the input image are treated as though they are composited onto an opaque black background.

Thanks to [Gisle Martens Meyer](https://community.vuo.org/u/gmm) for funding development of this node!
