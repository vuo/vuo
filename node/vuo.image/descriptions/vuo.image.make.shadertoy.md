Creates an image from GLSL source code.

[Shadertoy](http://www.shadertoy.com) is a website that allows developers to write and share small text programs called [Fragment Shaders](https://www.opengl.org/wiki/Fragment_Shader) that render 3D graphics to a screen.

Shadertoy provides some special `uniform` values that are exposed as port values on the node:

   - `Global Time` — The current time that is passed to the shader.  Usually you'll want to connect a `Render Image/Layers/Scene to Window` node's `Requested Frame` output to this port.
   - `Channel 0–3` — Images to be passed to the shader.
   - `Mouse Position` — The mouse position in Vuo Coordinates.
   - `Mouse Is Pressed` — Whether the mouse button is pressed.

To mimic the way the Shadertoy website handles interaction, connect the `Check Mouse Status` node's corresponding ports to this node.

All Shadertoy-specific inputs are supported, except `iChannelTime` and the ability to feed audio into the `iChannel` ports.

When typing in the `Fragment Shader` port's input editor, the Return and Tab keys type text as they normally would in a text editor (unlike most other text ports' input editors). To close the input editor, click away from it or press Command-Return.

### Troubleshooting

   - Make sure that you have images in the appropriate Channel ports.  If a shader makes use of an image and it is not provided, things may go awry.
   - If your screen is blank, the shader may contain an error.  In the event that a shader fails compilation, Vuo will print the error to the Console app.
   - Check that a time source is connected to the `Global Time` port.  Some shaders require a time base to function correctly.
   - If your shader code uses `iMouse`, make sure that you have connected the ports from `Check Mouse Status` to `Mouse Position` and `Mouse Is Pressed`.

### Shadertoy uniforms

   - `vec3 iResolution` — The output image size, in pixels.
   - `float iGlobalTime` — The value of the `Global Time` port.
   - `sampler2D iChannel[4]` — The texture sampler for each input image.
   - `vec3 iChannelResolution[4]` — The size of each input image, in pixels.
   - `vec4 iMouse`
      - `iMouse.xy` — When the mouse button is pressed, this is the current mouse position (in pixels).  Updates continuously while the mouse button is pressed; freezes when the mouse button is released.
      - `iMouse.zw` — When the mouse button is pressed, this is the position (in pixels) where the button was pressed.  When it's released, this is the position where it was released.
   - `vec4 iDate`
      - `iDate.x` — The current year.
      - `iDate.y` — The current month.
      - `iDate.z` — The current day.
      - `iDate.w` — The number of seconds since midnight.
