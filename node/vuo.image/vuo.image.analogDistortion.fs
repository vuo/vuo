/*{
	"ISFVSN": "2.0",
	"LABEL": "Add Analog Image Distortion",
	"VSN": "1.0.1",
	"LICENSE": "Copyright © 2012–2022 Kosada Incorporated.  This code may be modified and distributed under the terms of the MIT License.  For more information, see https://vuo.org/license.",
	"KEYWORDS": [
		"filter",
		"glitch", "artifact", "malfunction", "error", "scramble", "degrade",
		"lofi", "lo-fi", "low fidelity", "bad", "poor quality", "lossy",
		"television", "TV", "NTSC", "PAL", "broadcast", "video",
		"tape", "cassette", "videocassette", "VTR", "VHS", "Betamax",
		"static", "snow", "garbage", "random", "chrominance", "oscillation",
		"skew", "offset", "wrap", "repeat", "echo",
		"sync", "vsync",
	],
	"EXAMPLES": [ ],
	"TYPE":"IMAGE",
	"INPUTS":[
		{"NAME":"image",            "TYPE":"image"},
		{"NAME":"flagging",         "TYPE":"float", "DEFAULT":-0.5,   "MIN":-1,   "MAX":1,   "STEP":0.1  },
		{"NAME":"tearing",          "TYPE":"float", "DEFAULT": 0.1,   "MIN":-1,   "MAX":1,   "STEP":0.1  },
		{"NAME":"edgeBlanking",     "TYPE":"float", "DEFAULT": 0.04,  "MIN": 0,   "MAX":1,   "STEP":0.01 },
		{"NAME":"bearding",         "TYPE":"float", "DEFAULT": 0.1,   "MIN": 0,   "MAX":1,   "STEP":0.01 },
		{"NAME":"beardingDistance", "TYPE":"float", "DEFAULT": 0.005, "MIN":-0.1, "MAX":0.1, "STEP":0.001},
		{"NAME":"ghosting",         "TYPE":"float", "DEFAULT": 0.4,   "MIN": 0,   "MAX":1,   "STEP":0.01 },
		{"NAME":"ghostingDistance", "TYPE":"float", "DEFAULT": 0.03,  "MIN":-0.1, "MAX":0.1, "STEP":0.001},
		{"NAME":"ringing",          "TYPE":"float", "DEFAULT": 0.2,   "MIN": 0,   "MAX":1,   "STEP":0.01 },
		{"NAME":"noiseAmount",      "TYPE":"float", "DEFAULT": 0.2,   "MIN": 0,   "MAX":1,   "STEP":0.1  },
		{"NAME":"noiseScale",       "TYPE":"float", "DEFAULT": 0.005, "MIN": 0,   "MAX":0.1, "STEP":0.01 },
		{"NAME":"verticalOffset",   "TYPE":"float", "DEFAULT":-0.02,  "MIN":-1,   "MAX":1,   "STEP":0.01 },
	],
	"OUTPUTS":[{"NAME":"distortedImage"}],
}*/

#include "VuoGlslBrightness.glsl"
#include "VuoGlslRandom.glsl"
#include "noise3D.glsl"

uniform float aspectRatio;

float BlendColorDodgef(float base, float blend) { return ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0)); }
float BlendColorBurnf(float base, float blend) { return (blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0); }
float BlendVividLightf(float base, float blend) { return ((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5)))); }

void main()
{
	vec2 p = isf_FragNormCoord.xy;


	float variance0 = VuoGlsl_random1D1D( FRAMEINDEX      ) - .5;
	float variance1 = VuoGlsl_random1D1D(-FRAMEINDEX + p.y) - .5;


	// Flagging/Skewing/Tearing
	// https://bavc.github.io/avaa/artifacts/time_base_error.html
	// https://bavc.github.io/avaa/artifacts/skew_error.html
	if (flagging != 0.)
		p.x -= pow(clamp(     p.y-.9, 0, 1)*10, 4.) * flagging/5. * (1. + variance0/10.)
			 - pow(clamp(1. - p.y-.9, 0, 1)*10, 4.) * flagging/5. * (1. + variance0/10.);


	p.y -= verticalOffset;
	if (tearing != 0.)
		p.x -= VuoGlsl_brightness(IMG_NORM_PIXEL(image, p.yy * aspectRatio * (1. + variance1/200.)), 0) * tearing / 10. * (1. + variance0/20.);


	p = mod(p, 1.);
	vec4 color = IMG_NORM_PIXEL(image, p);


	if (p.x <      edgeBlanking/2. * (1. + variance1/8.)
	 || p.x > 1. - edgeBlanking/2. * (1. + variance1/8.))
		color = vec4(vec3(.07), 1.);


	// Bearding
	// https://bavc.github.io/avaa/artifacts/bearding.html
	// If a pixel left of us is too bright, that can cause us to dim.
	if (bearding > 0.)
	{
		vec4 beardingSource = IMG_NORM_PIXEL(image, p - vec2(beardingDistance * 2., 0.));
		if (VuoGlsl_brightness(beardingSource, 0) > (1. - pow(bearding, 4.)) * (1. + variance1/500.))
		{
			color.rgb -= beardingSource.rgb;
			color.rgb = clamp(color.rgb, 0.1, 1);
		}
	}


	// Ghosting
	// https://bavc.github.io/avaa/artifacts/ghost.html
	if (ghosting > 0.)
	{
		int ghosts = 5;
		for (int i = 1; i <= ghosts; ++i)
		{
			vec4 ghostSource = IMG_NORM_PIXEL(image, p - vec2(ghostingDistance * i * 2. * (1. + variance0/50.), 0.));
			vec3 blended = vec3(
				BlendVividLightf(color.r, ghostSource.r),
				BlendVividLightf(color.g, ghostSource.g),
				BlendVividLightf(color.b, ghostSource.b));
			color.rgb = mix(color.rgb, blended, pow((ghosts + 2 - i) * ghosting / ghosts / 2., 2.));
		}
	}


	// Ringing
	// https://bavc.github.io/avaa/artifacts/ringing.html
	// Like ghosting, but a smaller (1-pixel) offset, and alternately inverted.
	if (ringing > 0.)
	{
		int rings = 8;
		float ringDistance = 1. / IMG_SIZE(image).x;
		vec3 origColor = color.rgb;
		for (int i = 1; i <= rings; ++i)
		{
			vec4 ringSource = IMG_NORM_PIXEL(image, p - vec2(ringDistance * i, 0.));
			float a = (rings + 1 - i) * (ringing * 4.) / rings;
			color.rgb += (mod(i, 2) == 0 ? ringSource.rgb : -ringSource.rgb) * a * (1. + variance0/20.);
		}
		color.rgb += origColor/2. * (ringing * 4.);
		color.rgb = clamp(color.rgb, 0, 1);
	}


	// Chrominance Noise
	// https://bavc.github.io/avaa/artifacts/chrominance_noise.html
	// Slightly horizontally stretched.  Most visible in dark areas.
	float scale = 1. / max(0.000001, noiseScale);
	vec3 chromaNoise = snoise3D3D(vec3((p.x - .5) * scale/3, (p.y - .5) * scale / aspectRatio, FRAMEINDEX));
	float brightness = VuoGlsl_brightness(color, 0);
	color.rgb += chromaNoise * (1. - pow(brightness, .1)) * max(0., noiseAmount) * (1. + variance1/10.);


	gl_FragColor = color;
}
