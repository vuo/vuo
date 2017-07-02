These nodes are for working with colors. 

A color can be represented in various ways by these nodes:

   - The [HSL color model](http://en.wikipedia.org/wiki/HSL_and_HSV) corresponds to the way that many people think about colors. An HSL color is represented by three numbers ranging from 0 to 1. The first number represents the [hue](http://simple.wikipedia.org/wiki/Hue), the second number represents the [saturation](http://en.wikipedia.org/wiki/Colorfulness), and the third number represents the [lightness](http://en.wikipedia.org/wiki/Lightness).
      - Example: Bright red has HSL value [0, 1, 0.5].
   - The [RGB color model](http://en.wikipedia.org/wiki/RGB_color_model) is often used to represent colors on a computer. An RGB color is represented by three numbers ranging from 0 to 1. The first number represents the amount of red, the second number represents the amount of green, and the third color represents the amount of blue. The red, green, and blue components of an RGB color are combined [additively](http://en.wikipedia.org/wiki/Additive_color), so the result is like mixing light.
      - Example: Bright red has RGB value [1, 0, 0].
      - Example: Pale yellow has RGB value [1, 1, 0.5].
   - The [CMYK color model](https://en.wikipedia.org/wiki/CMYK_color_model) is often used to represent colors in printing. A CMYK color is represented by four numbers ranging from 0 to 1. The first number represents the amount of cyan, the second number represents the amount of magenta, the third number represents the amount of yellow, and the fourth color represents the amount of black (also called *key*). The cyan, magenta, yellow, and black components of a CMYK color are combined [subtractively](https://en.wikipedia.org/wiki/Subtractive_color), so the result is like mixing ink.
      - Example: Bright red has CMYK value [0, 1, 1, 0].
      - Example: Pale yellow has CMYK value [0, 0, 0.5, 0].

These nodes treat colors the same regardless of whether they were created using HSL, RGB, or CMYK. For example, you can connect the output port of a `Make HSL Color` node to any input port that accepts a color, including a `Get RGB Color Values` node.

In addition to the numbers representing HSL, RGB, or CMYK, a color can be described by a fourth number representing its **opacity** (also known as *alpha*). This number also ranges from 0 to 1, with 1 being fully opaque and 0 being fully transparent.
