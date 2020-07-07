Creates a color from component values in a particular colorspace.

   - `Colorspace` —
      - When specialized to Integer, you can select one of the built-in colorspaces.
      - When specialized to Data, you can use [Fetch Data](vuo-node://vuo.data.fetch) to load an ICC profile.
      - See the [vuo.color](vuo-nodeset://vuo.color) node set documentation for more information.
   - `Components` — A list containing each of the component values in the specified colorspace, including alpha (opacity).  The meaning of each component depends on the selected colorspace.  This can range from 2 items (luminance + opacity for grayscale colorspaces) to 5 items (four components + opacity for CMYK colorspaces).  If you provide fewer values than the colorspace requires, the missing component values default to 0 and alpha defaults to 1 (fully opaque).
   - `Color` — The resulting color, in Vuo's native sRGB colorspace.
