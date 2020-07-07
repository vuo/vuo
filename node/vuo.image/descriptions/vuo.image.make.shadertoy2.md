Creates an image from Shadertoy's variant of GLSL source code.

[Shadertoy](http://www.shadertoy.com) is a website that allows developers to write and share small text programs called [Fragment Shaders](https://www.opengl.org/wiki/Fragment_Shader) that render graphics to a screen.

Shadertoy provides some special `uniform` values that are exposed as port values on the node:

   - `Time` — The current time that is passed to the shader.  Usually you'll want to connect a [Fire on Display Refresh](vuo-node://vuo.event.fireOnDisplayRefresh) node's `Refreshed at Time` output to this port.
   - `Channel 0–3` — Images to be passed to the shader.
   - `Mouse Position` — The mouse position in Vuo Coordinates.
   - `Mouse is Pressed` — Whether the mouse button is pressed.

To mimic the way the Shadertoy website handles interaction, connect the [Check Mouse Status](vuo-node://vuo.mouse.status2) node's corresponding ports to this node.

When typing in the `Fragment Shader` port's input editor, the Return and Tab keys type text as they normally would in a text editor (unlike most other text ports' input editors). To close the input editor, click away from it or press Command-Return.

### Troubleshooting

   - Make sure that you have images in the appropriate Channel ports.  If a shader makes use of an image and it is not provided, things may go awry.
   - If your screen is blank, the shader may contain an error.  In the event that a shader fails compilation, Vuo will print the error to the Console app.
   - Check that a time source is connected to the `Time` port.  Some shaders require a time base to function correctly.
   - If your shader code uses `iMouse`, make sure that you have connected the ports from [Check Mouse Status](vuo-node://vuo.mouse.status2) to `Mouse Position` and `Mouse is Pressed`.
   - If using a shader downloaded from the Shadertoy website, check that the the inputs to each `Channel` port in Vuo match those on the website.
   - If an input image's wrap mode is Repeat on the Shadertoy website, use the [Change Wrap Mode](vuo-node://vuo.image.wrapMode) node to make it repeat in Vuo.
   - If the shader calls the `textureLod` function, use the [Improve Downscaling Quality](vuo-node://vuo.image.mipmap) node to mipmap the input image.

### Supported features

This node only supports shaders that have a single Image tab. It doesn't support shaders with additional tabs (Buffer, Sound, Common, Cubemap).

This node only supports images (textures) as `Channel` inputs. It doesn't support audio or other types of inputs.

This node uses GLSL 1.20.  Features that were added in later versions of GLSL, such as `switch` statements, may not be supported.

### Shadertoy uniforms

   - `vec3 iResolution` — The output image size, in pixels.
   - `float iTime` — The value of the `Time` port.
   - `float iTimeDelta` — The elapsed `Time` since the prior frame.
   - `int iFrame` — The number of frames rendered since the composition started.
   - `sampler2D iChannel0, iChannel1, iChannel2, iChannel3` — The texture sampler for each input image.
   - `vec3 iChannelResolution[4]` — The size of each input image, in pixels.
   - `vec4 iMouse`
      - `iMouse.xy` — When the mouse button is pressed, this is the current mouse position (in pixels).  Updates continuously while the mouse button is pressed; freezes when the mouse button is released.
      - `iMouse.zw` — When the mouse button is pressed, this is the position (in pixels) where the button was pressed.  When it's released, this is the position where it was released.
   - `vec4 iDate`
      - `iDate.x` — The current year.
      - `iDate.y` — The current month.
      - `iDate.z` — The current day.
      - `iDate.w` — The number of seconds (including fractional seconds) since midnight.
