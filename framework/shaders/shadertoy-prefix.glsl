#version 120

// Some popular shaders, such as https://www.shadertoy.com/view/4tVBzD,
// use bitwise integer operators, which aren't available until GLSL 1.30.
// This extension adds support for them.
// Apple claims this extension is supported on all GPUs as of Mac OS X 10.9.
#extension GL_EXT_gpu_shader4 : enable

uniform vec3        iResolution;                  // viewport resolution (in pixels)
uniform float       iTime;                        // shader playback time (in seconds)
#define iGlobalTime iTime
uniform vec4        iMouse;                       // mouse pixel coords
uniform vec4        iDate;                        // (year, month, day, time in seconds)
uniform float       iSampleRate;                  // sound sample rate (i.e., 44100)
uniform vec3        iChannelResolution[4];        // channel resolution (in pixels)
uniform float       iChannelTime[4];              // channel playback time (in sec)

uniform vec2        ifFragCoordOffsetUniform;     // used for tiled based hq rendering
uniform float       iTimeDelta;                   // render time (in seconds)
uniform int         iFrame;                       // shader playback frame
uniform float       iFrameRate;

struct Channel {
    vec3    resolution;
    float   time;
};

uniform Channel iChannel[4];

uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

// Some functions only available in GLSL versions later than 1.20:
vec4 textureGrad(sampler2D sampler, vec2 P, vec2 dPdx, vec2 dPdy)
{
    return texture2DGrad(sampler, P, dPdx, dPdy);
}
vec4 textureLod(sampler2D sampler, vec2 P, float lod)
{
    return texture2DLod(sampler, P, lod);
}
vec4 textureLod(sampler3D sampler, vec3 P, float lod)
{
    return texture3DLod(sampler, P, lod);
}
vec4 texture(sampler2D sampler, vec2 P)
{
    return texture2D(sampler, P);
}
vec4 texture(sampler3D sampler, vec3 P)
{
    return texture3D(sampler, P);
}
vec4 texture(sampler3D sampler, vec3 P, float lod)
{
    return texture3DLod(sampler, P, lod);
}
vec4 texture(sampler2D sampler, vec2 P, float bias)
{
    return texture2D(sampler, P, bias);
}
ivec2 textureSize(sampler2D sampler, int lod)
{
	return textureSize2D(sampler, lod);
}
vec4 texelFetch(sampler2D sampler, ivec2 P, int lod)
{
	return texelFetch2D(sampler, P, lod);
}
bool isnan(float f)
{
    return !(f < 0. || f > 0. || f == 0.);
}
bool isinf(float f)
{
    return abs(f) > 1./0.;
}

// Since we don't yet support 3D textures, fudge them:
vec4 texture(sampler2D sampler, vec3 P)
{
    return texture2D(sampler, P.xy);
}
vec4 textureLod(sampler2D sampler, vec3 P, float lod)
{
    return texture2DLod(sampler, P.xy, lod);
}
vec4 texture(sampler2D sampler, vec3 P, float lod)
{
    return texture2DLod(sampler, P.xy, lod);
}

void mainImage(inout vec4 fragColor, vec2 fragCoord);
