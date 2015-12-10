These nodes are for working with colors. 

A color can be described in two different ways by these nodes: **HSL (hue, saturation, lightness)** or **RGB (red, green, blue)**. In each case, the color is described by a group of three numbers, with each number between 0 and 1.

For HSL, the first number represents the [hue](http://simple.wikipedia.org/wiki/Hue), the second number represents the [saturation](http://en.wikipedia.org/wiki/Colorfulness), and the third number represents the [lightness](http://en.wikipedia.org/wiki/Lightness). For example, bright red has a hue of 0, a saturation of 1, and a lightness of 0.5.

For RGB, the first number represents the amount of red, the second number represents the amount of green, and the third color represents the amount of blue. For example, bright red has a red value of 1, a green value of 0, and a blue value of 0. The red, green, and blue components of an RGB color are combined [additively](http://en.wikipedia.org/wiki/Additive_color), so the result is like mixing light (as opposed to mixing paints). For example, a color with a red value of 1, a green value of 1, and a blue value of 0.5 would be pale yellow (not dark brown).

The [HSL color model](http://en.wikipedia.org/wiki/HSL_and_HSV) is closer to how people typically think about colors. The [RGB color model](http://en.wikipedia.org/wiki/RGB_color_model) is often used to represent colors on a computer.

These nodes treat colors the same regardless of whether they were created using HSL or RGB. For example, you can connect the output port of a `Make HSL Color` node to any input port that accepts a color, including a `Get RGB Color Values` node.

In addition to the three numbers representing HSL or RGB, a color can be described by a fourth number representing its **opacity** (also known as *alpha*). This number also ranges from 0 to 1, with 1 being fully opaque and 0 being fully transparent.
