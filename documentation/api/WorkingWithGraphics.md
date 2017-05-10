@addtogroup WorkingWithGraphics

Major data types for working with graphics:

   - @ref VuoImage
   - @ref VuoImageRenderer
   - @ref VuoSceneObject
   - @ref VuoSceneRenderer
   - @ref VuoSceneObjectRenderer
   - @ref VuoShader
   - @ref VuoLayer

## Alpha premultiplication

When performing calculations on colors in shaders and other graphics code, it's important to know if the RGB values are premultiplied. Vuo uses premultiplied colors in some situations and straight (non-premultiplied) colors in others.

A premultiplied RGBA color has had its RGB values multiplied by its A (alpha) value. For example, the straight representation of yellow with 50% opacity is (1.0, 1.0, 0.0, 0.5), and the premultiplied representation of the same color is (0.5, 0.5, 0.0, 0.5).

Notice that there's no way to tell just from looking at the RGBA values if the above color, (0.5, 0.5, 0.0, 0.5), is premultiplied or straight. If premultiplied, it would be a bright yellow. If straight, it would be a dark yellow.

In Vuo, you can tell if a color is premultiplied based on the situation. There are 3 rules to follow to make sure that your code correctly interprets and outputs colors as premultiplied or straight:

   1. VuoColors expect and provide non-premultiplied RGB values.
   2. Images (textures) and framebuffers are premultiplied.
      - VuoImages expect and provide premultiplied RGB values in CPU buffers, and textures provide premultiplied RGB values when sampled in GLSL.
      - Colors written to the GLSL framebuffer (`gl_FragColor`) should be premultiplied.
      - `VuoShader_setUniform_VuoColor` premultiplies the color before passing it to the shader.
   3. Typically you should blend premultiplied colors (so transparent colors don't unduly affect the result).
