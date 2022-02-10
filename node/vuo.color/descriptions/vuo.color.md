Nodes for creating, deconstructing, and mixing colors.

## Color Models
A color can be represented in various ways by these nodes:

   - The [HSL color model](https://en.wikipedia.org/wiki/HSL_and_HSV) corresponds to the way that many people think about colors. An HSL color is represented by three numbers ranging from 0 to 1. The first number represents the [hue](http://simple.wikipedia.org/wiki/Hue), the second number represents the [saturation](https://en.wikipedia.org/wiki/Colorfulness), and the third number represents the [lightness](https://en.wikipedia.org/wiki/Lightness).
      - Example: Bright red has HSL value [0, 1, 0.5].
   - The [RGB color model](https://en.wikipedia.org/wiki/RGB_color_model) is often used to represent colors on a computer. An RGB color is represented by three numbers ranging from 0 to 1. The first number represents the amount of red, the second number represents the amount of green, and the third color represents the amount of blue. The red, green, and blue components of an RGB color are combined [additively](https://en.wikipedia.org/wiki/Additive_color), so the result is like mixing light.
      - Example: Bright red has RGB value [1, 0, 0].
      - Example: Pale yellow has RGB value [1, 1, 0.5].
   - The [CMYK color model](https://en.wikipedia.org/wiki/CMYK_color_model) is often used to represent colors in printing. A CMYK color is represented by four numbers ranging from 0 to 1. The first number represents the amount of cyan, the second number represents the amount of magenta, the third number represents the amount of yellow, and the fourth color represents the amount of black (also called *key*). The cyan, magenta, yellow, and black components of a CMYK color are combined [subtractively](https://en.wikipedia.org/wiki/Subtractive_color), so the result is like mixing ink.
      - Example: Bright red has CMYK value [0, 1, 1, 0].
      - Example: Pale yellow has CMYK value [0, 0, 0.5, 0].
   - Vuo's [Get ICC Color Values](vuo-node://vuo.color.get.icc) and [Make ICC Color](vuo-node://vuo.color.make.icc) nodes can convert colors to and from other colorspaces, such as [CIE L*a*b*](https://en.wikipedia.org/wiki/CIELAB_color_space) and [CIE XYZ](http://www.techmind.org/colour/), or any other ICC profile such as those provided by the [International Color Consortium](http://www.color.org/registry/) and [Adobe](https://supportdownloads.adobe.com/detail.jsp?ftpID=3680), and those bundled with printers, scanners, cameras, and displays.
      - To get information about an ICC color profile, such as how many components it uses, double-click the ICC file in Finder, and it will open in the macOS ColorSync Utility.  In the "Header" section, the "Space" field shows the number and meaning of the profile's components.

Vuo natively stores colors as floating-point values in the [sRGB colorspace](https://en.wikipedia.org/wiki/SRGB), and treats colors the same regardless of whether they were created using HSL, RGB, or CMYK.  For example, you can connect the output port of a [Make HSL Color](vuo-node://vuo.color.make.hsl) node to any input port that accepts a color, including a [Get RGB Color Values](vuo-node://vuo.color.get.rgb) node.  Vuo can also represent non-sRGB colors by using values outside the standard 0–1 sRGB range.

## Opacity
In addition to the numbers representing HSL, RGB, or CMYK, a color can be described by another number representing its **opacity** (also known as *alpha*). This number also ranges from 0 to 1, with 1 being fully opaque and 0 being fully transparent. When opacity is included in a color model, the abbreviations are HSLA, RGBA, and CMYKA, respectively.

## Sorting
For nodes such as [Sort List](vuo-node://vuo.list.sort) and [Is Less Than](vuo-node://vuo.data.isLessThan) that impose an ordering among colors, the sort is performed primarily on the colors' respective R-values, secondarily on their G-values, next on their B-values, and finally on their opacity values.
