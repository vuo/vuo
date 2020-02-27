See [Turning graphics shaders into nodes](https://doc.vuo.org/latest/manual/turning-graphics-shaders-into-nodes.xhtml) for more information.

## GLSL 1.20

See the [OpenGL Reference](https://www.khronos.org/registry/OpenGL-Refpages/gl4/) for more information on these functions and variables.  Also, [The Book of Shaders](https://thebookofshaders.com/) provides an introduction to GLSL fragment shaders and some rendering techniques.

### Data types

    float, vec2, vec3, vec4
    int, ivec2, ivec3, ivec4
    bool, bvec2, bvec3, bvec4
    mat2, mat3, mat4

### Functions

    sin, cos, tan
    asin, acos, atan
    radians, degrees
    pow, exp, log, exp2, log2
    sqrt, inversesqrt
    mod, ceil, floor, fract, sign
    max, min
    abs, clamp
    mix, step, smoothstep
    dot, cross
    distance, length, normalize
    faceforward
    reflect, refract
    fwidth, dFdx, dFdy
    matrixCompMult
    all, any, equal, not, notEqual
    greaterThan, greaterThanEqual
    lessThan, lessThanEqual
    noise1, noise2, noise3, noise4

### Fragment shader variables

Input:

    vec4 gl_FragCoord
    bool gl_FrontFacing

Output:

    vec4 gl_FragColor
    float gl_FragDepth

## ISF extensions

See [How ISF source code translates to a Vuo node](https://doc.vuo.org/latest/manual/how-isf-source-code-translates-to-a-vuo-node.xhtml) for more information.

### Functions

    vec4 IMG_PIXEL(image, vec2)
    vec4 IMG_NORM_PIXEL(image, vec2)
    vec4 IMG_THIS_PIXEL(image)
    vec4 IMG_THIS_NORM_PIXEL(image)
    vec2 IMG_SIZE(image)
    int  LIST_LENGTH(list)

### Uniforms

    vec2  RENDERSIZE
    float TIME
    float TIMEDELTA
    vec2  isf_FragNormCoord
    vec4  DATE
    int   FRAMEINDEX

## Vuo extensions

Vuo provides several libraries of GLSL functions:

    #include "VuoGlslBrightness.glsl"
    float VuoGlsl_brightness(vec4 color, VuoThresholdType type)

    #include "VuoGlslHsl.glsl"
    vec3  VuoGlsl_rgbToHsl(vec3 color)
    vec3  VuoGlsl_hslToRgb(vec3 hsl)

    #include "VuoGlslRandom.glsl"
    float VuoGlsl_random1D1D(float v)
    float VuoGlsl_random2D1D(vec2 v)
    float VuoGlsl_random3D1D(vec3 v)
    vec3  VuoGlsl_random2D3D(vec2 v)
    vec2  VuoGlsl_random3D2D(vec3 v)
    vec3  VuoGlsl_random3D3D(vec3 v)

    #include "GPUNoiseLib.glsl"
    float Value2D(vec2 v)
    float Value3D(vec3 v)
    float Value4D(vec4 v)
    float Perlin2D(vec2 v)
    float Perlin3D(vec3 v)
    float Perlin4D(vec4 v)
    float Cellular2D(vec2 v)
    float Cellular3D(vec3 v)
    float SimplexPerlin2D(vec2 v)
    float SimplexPerlin3D(vec3 v)
    float SimplexCellular2D(vec2 v)
    float SimplexCellular3D(vec3 v)

    #include "noise2D.glsl"
    float snoise2D1D(vec2 v)
    vec2  snoise2D2D(vec2 v)
    vec3  snoise2D3D(vec2 v)
    vec4  snoise2D4D(vec2 v)

    #include "noise3D.glsl"
    float snoise3D1D(vec3 v)
    float snoise3D1DFractal(vec3 v, int levels, float roughness, float spacing)
    vec2  snoise3D2D(vec3 v)
    vec2  snoise3D2DFractal(vec3 v, int levels, float roughness, float spacing)
    vec3  snoise3D3D(vec3 v)
    vec4  snoise3D4D(vec3 v)
    float cnoise3D1D(vec3 v)
    float cnoise3D1DFractal(vec3 v, int levels, float roughness, float spacing)
    float pnoise3D1D(vec3 v, vec3 rep)

    #include "noise4D.glsl"
    float snoise4D1D(vec4 v)
    vec2  snoise4D2D(vec4 v)
    vec3  snoise4D3D(vec4 v)
    vec4  snoise4D4D(vec4 v)
