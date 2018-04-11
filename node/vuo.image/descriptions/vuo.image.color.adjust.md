Alters an image's color characteristics. 

   - `Saturation` — The intensity of colors. At -1, the image is grayscale. As saturation increases, the colors become more intense. 
   - `Vibrance` — Like Saturation, but mostly affects parts of the image that start out with low saturation.
   - `Hue Shift` — The number of degrees to shift the colors around the red-yellow-green-cyan-blue-magenta color circle. At 60°, red becomes yellow, yellow becomes green, and so on. At 120°, red becomes green, yellow becomes cyan, and so on.
   - `Temperature` — Adjusts the image's white balance.  At -1 the image appears cooler (blueish), and at 1 the image appears warmer (orangish).
   - `Tint` — Shifts the image's colors toward green (-1) or magenta (1).
   - `Contrast` — The difference between light and dark colors in the image. At -1, the image is all the same color. As contrast increases, the dark colors become darker and the light colors become lighter. 
   - `Brightness` — The darkness or lightness of colors. At -1, the image is black. At 1, the image is white. 
   - `Exposure` — The darkness or lightness of colors, especially affecting the lightest colors (highlights). At -1, the image is black. At 1, the image is white. 
   - `Gamma` — The darkness or lightness of colors, especially affecting the medium-brightness colors (midtones). At 1, the image is unchanged. At 0, the image is white. As gamma increases, the image becomes darker. 

Thanks to [Brad Larson](https://github.com/BradLarson/GPUImage) and [Martinus Magneson](https://vuo.org/user/3272) for implementing parts of the GLSL code used in this node.
