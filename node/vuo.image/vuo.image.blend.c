/**
 * @file
 * vuo.image.blend node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoBlendMode.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Blend Images",
					 "keywords" : [ "combine", "mix", "fade", "merge", "layer", "composite", "channel",
						"normal", "add", "additive", "alpha", "opacity", "transparent", "transparency",
						"multiply", "darker", "linear burn", "color burn", "burn",
						"screen", "lighter", "linear dodge", "color dodge", "dodge",
						"overlay", "soft light", "hard light", "vivid light", "linear light", "pin light", "light", "hard mix",
						"difference", "exclusion", "subtract", "divide",
						"hue", "saturation", "desaturate", "grayscale", "greyscale", "color", "luminosity", "filter" ],
					 "version" : "1.2.1",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "BlendImages.vuo", "SimulateFilmProjector.vuo" ]
					 }
				 });


#define FilterSource "\
	uniform sampler2D background;\
	uniform sampler2D foreground;\
	uniform float foregroundOpacity;\
	varying vec4 fragmentTextureCoordinate;\
	\
	vec4 Desaturate(vec3 color, float Desaturation)\
	{\
		vec3 grayXfer = vec3(0.3, 0.59, 0.11);\
		vec3 gray = vec3(dot(grayXfer, color));\
		return vec4(mix(color, gray, Desaturation), 1.0);\
	}\
	\
	float BlendAddf(float base, float blend) { return min(base + blend, 1.0); }\
	float BlendLinearDodgef(float base, float blend) { return min(base + blend, 1.0); }\
	float BlendSubstractf(float base, float blend) { return max(base + blend - 1.0, 0.0); }\
	float BlendLinearBurnf(float base, float blend) { return max(base + blend - 1.0, 0.0); }\
	\
	float BlendLightenf(float base, float blend) { return max(blend, base); }\
	float BlendDarkenf(float base, float blend) { return min(blend, base); }\
	float BlendLinearLightf(float base, float blend) { return (blend < 0.5 ? BlendLinearBurnf(base, (2.0 * blend)) : BlendLinearDodgef(base, (2.0 * (blend - 0.5)))); }\
	\
	float BlendScreenf(float base, float blend) { return (1.0 - ((1.0 - base) * (1.0 - blend))); }\
	float BlendOverlayf(float base, float blend) { return (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend))); }\
	float BlendSoftLightf(float base, float blend) { return ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend))); }\
	float BlendColorDodgef(float base, float blend) { return ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0)); }\
	float BlendColorBurnf(float base, float blend) { return (blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0); }\
	\
	float BlendVividLightf(float base, float blend) { return ((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5)))); }\
	float BlendPinLightf(float base, float blend) { return ((blend < 0.5) ? BlendDarkenf(base, (2.0 * blend)) : BlendLightenf(base, (2.0 *(blend - 0.5)))); }\
	float BlendHardMixf(float base, float blend) { return ((BlendVividLightf(base, blend) < 0.5) ? 0.0 : 1.0); }\
	float BlendReflectf(float base, float blend) { return ((blend == 1.0) ? blend : min(base * base / (1.0 - blend), 1.0)); }\
	\
	include(hsl)\
	"

#define BLEND_FILTERS(source) "#version 120\n" FilterSource "\n" #source

static const char * fragmentShaderSource_blendNormal = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);
		blend.rgb /= (blend.a == 0.) ? 1. : blend.a;	// un-premultiply
		float foregroundAlpha = blend.a * foregroundOpacity;
		gl_FragColor = vec4(mix(base.rgb, blend.rgb, foregroundAlpha),
							mix(base.a,   1.,        foregroundAlpha));
	}

);

static const char * fragmentShaderSource_blendMultiply = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( base.r * blend.r,
							base.g * blend.g,
							base.b * blend.b );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendLinearDodge = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	BlendLinearDodgef(base.r, blend.r),
							BlendLinearDodgef(base.g, blend.g),
							BlendLinearDodgef(base.b, blend.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendDarkerComponent = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendDarkerColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 base_lum = rgbToHsl(base.rgb);
		vec3 blend_lum = rgbToHsl(blend.rgb);

		vec3 result = base_lum.b < blend_lum.b ? base.rgb : blend.rgb;

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}

);

static const char * fragmentShaderSource_blendLighterComponent = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLighterColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 base_lum = rgbToHsl(base.rgb);
		vec3 blend_lum = rgbToHsl(blend.rgb);

		vec3 result = base_lum.b > blend_lum.b ? base.rgb : blend.rgb;

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLinearBurn = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	base.r + blend.r - 1.,
							base.g + blend.g - 1.,
							base.b + blend.b - 1. );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColorBurn = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// 1-(1-b)/f
		vec3 result = vec3(	1-min((1-base.r)/blend.r,1.),
							1-min((1-base.g)/blend.g,1.),
							1-min((1-base.b)/blend.b,1.) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColorDodge = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// b/(1-f)
		vec3 result = vec3(	base.r / (1-blend.r),
							base.g / (1-blend.g),
							base.b / (1-blend.b) );

		result = clamp(result, 0., 1.);

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendScreen = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// 1-(1-b)•(1-f)
		vec3 result = vec3(	1-(1-base.r) * (1-blend.r),
							1-(1-base.g) * (1-blend.g),
							1-(1-base.b) * (1-blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendOverlay = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend)))
		vec3 result = vec3(	(base.r < 0.5 ? (2.0 * base.r * blend.r) : (1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r))),
							(base.g < 0.5 ? (2.0 * base.g * blend.g) : (1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g))),
							(base.b < 0.5 ? (2.0 * base.b * blend.b) : (1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b))) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSoftLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend)))

		vec3 result = vec3(	((blend.r < 0.5) ? (2.0 * base.r * blend.r + base.r * base.r * (1.0 - 2.0 * blend.r)) : (sqrt(base.r) * (2.0 * blend.r - 1.0) + 2.0 * base.r * (1.0 - blend.r))),
							((blend.g < 0.5) ? (2.0 * base.g * blend.g + base.g * base.g * (1.0 - 2.0 * blend.g)) : (sqrt(base.g) * (2.0 * blend.g - 1.0) + 2.0 * base.g * (1.0 - blend.g))),
							((blend.b < 0.5) ? (2.0 * base.b * blend.b + base.b * base.b * (1.0 - 2.0 * blend.b)) : (sqrt(base.b) * (2.0 * blend.b - 1.0) + 2.0 * base.b * (1.0 - blend.b))) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHardLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3(	(blend.r < 0.5 ? (2.0 * blend.r * base.r) : (1.0 - 2.0 * (1.0 - blend.r) * (1.0 - base.r))),
							(blend.g < 0.5 ? (2.0 * blend.g * base.g) : (1.0 - 2.0 * (1.0 - blend.g) * (1.0 - base.g))),
							(blend.b < 0.5 ? (2.0 * blend.b * base.b) : (1.0 - 2.0 * (1.0 - blend.b) * (1.0 - base.b))) );


		result = mix(blend.rgb, result, base.a);
		result = mix(base.rgb, result, blend.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendVividLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(((blend.r < 0.5) ? BlendColorBurnf(base.r, (2.0 * blend.r)) : BlendColorDodgef(base.r, (2.0 * (blend.r - 0.5)))),
							((blend.g < 0.5) ? BlendColorBurnf(base.g, (2.0 * blend.g)) : BlendColorDodgef(base.g, (2.0 * (blend.g - 0.5)))),
							((blend.b < 0.5) ? BlendColorBurnf(base.b, (2.0 * blend.b)) : BlendColorDodgef(base.b, (2.0 * (blend.b - 0.5)))) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLinearLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendLinearLightf(base.r, blend.r),
							BlendLinearLightf(base.g, blend.g),
							BlendLinearLightf(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendPinLight = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendPinLightf(base.r, blend.r),
							BlendPinLightf(base.g, blend.g),
							BlendPinLightf(base.b, blend.b) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHardMix = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(BlendHardMixf(base.r, blend.r),
							BlendHardMixf(base.g, blend.g),
							BlendHardMixf(base.b, blend.b) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendDifference = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3(abs(base.r - blend.r),
							abs(base.g - blend.g),
							abs(base.b - blend.b) );


		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendExclusion = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		// b+f-2•b•f
		 vec3 result = vec3( base.r+blend.r - 2*base.r*blend.r,
							 base.g+blend.g - 2*base.g*blend.g,
							 base.b+blend.b - 2*base.b*blend.b );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSubtract = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 result = vec3( clamp(base.r - blend.r, 0., 1.),
							clamp(base.g - blend.g, 0., 1.),
							clamp(base.b - blend.b, 0., 1.) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendDivide = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		 vec3 result = vec3( clamp(base.r / blend.r, 0., 1.),
							 clamp(base.g / blend.g, 0., 1.),
							 clamp(base.b / blend.b, 0., 1.) );

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendHue = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = rgbToHsl(base.rgb);
		vec3 result = hslToRgb(vec3(rgbToHsl(blend.rgb).r, baseHSL.g, baseHSL.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendSaturation = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = rgbToHsl(base.rgb);
		vec3 result = hslToRgb(vec3(baseHSL.r, rgbToHsl(blend.rgb).g, baseHSL.b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendColor = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 blendHSL = rgbToHsl(blend.rgb);
		vec3 result = hslToRgb(vec3(blendHSL.r, blendHSL.g, rgbToHsl(base.rgb).b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

static const char * fragmentShaderSource_blendLuminosity = BLEND_FILTERS(

	void main()
	{
		vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
		vec4 blend = texture2D(foreground, fragmentTextureCoordinate.xy);

		vec3 baseHSL = rgbToHsl(base.rgb);
		vec3 result = hslToRgb(vec3(baseHSL.r, baseHSL.g, rgbToHsl(blend.rgb).b));

		result = mix(base.rgb, result, blend.a);
		result = mix(blend.rgb, result, base.a);

		gl_FragColor = vec4( mix(base.rgb, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
	VuoInteger currentBlendMode;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->glContext = VuoGlContext_use();

	instance->currentBlendMode = -1;	// set this to a negative value on initialization, forcing shader init on first event.

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) background,
		VuoInputData(VuoImage) foreground,
		VuoInputData(VuoBlendMode, {"default":"normal"}) blendMode,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) foregroundOpacity,
		VuoOutputData(VuoImage) blended
)
{
	if (!background && !foreground)
	{
		*blended = NULL;
		return;
	}

	// if the blend mode has changed or is uninitialized, init here
	if( blendMode != (*instance)->currentBlendMode )
	{
		// if this is the first time initializing, need to init the imagerenderer too
		if( (*instance)->currentBlendMode < 0 )
		{
			(*instance)->imageRenderer = VuoImageRenderer_make((*instance)->glContext);
			VuoRetain((*instance)->imageRenderer);
		}

		const char *fragmentShaderSource = fragmentShaderSource_blendNormal;
		switch(blendMode)
		{
			case VuoBlendMode_Normal:
				fragmentShaderSource = fragmentShaderSource_blendNormal;
				break;
			case VuoBlendMode_Multiply:
				fragmentShaderSource = fragmentShaderSource_blendMultiply;
				break;
			case VuoBlendMode_DarkerComponent:
				fragmentShaderSource = fragmentShaderSource_blendDarkerComponent;
				break;
			case VuoBlendMode_DarkerColor:
				fragmentShaderSource = fragmentShaderSource_blendDarkerColor;
				break;
			case VuoBlendMode_LinearBurn:
				fragmentShaderSource = fragmentShaderSource_blendLinearBurn;
				break;
			case VuoBlendMode_ColorBurn:
				fragmentShaderSource = fragmentShaderSource_blendColorBurn;
				break;
			case VuoBlendMode_Screen:
				fragmentShaderSource = fragmentShaderSource_blendScreen;
				break;
			case VuoBlendMode_LighterComponent:
				fragmentShaderSource = fragmentShaderSource_blendLighterComponent;
				break;
			case VuoBlendMode_LighterColor:
				fragmentShaderSource = fragmentShaderSource_blendLighterColor;
				break;
			case VuoBlendMode_LinearDodge:
				fragmentShaderSource = fragmentShaderSource_blendLinearDodge;
				break;
			case VuoBlendMode_ColorDodge:
				fragmentShaderSource = fragmentShaderSource_blendColorDodge;
				break;
			case VuoBlendMode_Overlay:
				fragmentShaderSource = fragmentShaderSource_blendOverlay;
				break;
			case VuoBlendMode_SoftLight:
				fragmentShaderSource = fragmentShaderSource_blendSoftLight;
				break;
			case VuoBlendMode_HardLight:
				fragmentShaderSource = fragmentShaderSource_blendHardLight;
				break;
			case VuoBlendMode_VividLight:
				fragmentShaderSource = fragmentShaderSource_blendVividLight;
				break;
			case VuoBlendMode_LinearLight:
				fragmentShaderSource = fragmentShaderSource_blendLinearLight;
				break;
			case VuoBlendMode_PinLight:
				fragmentShaderSource = fragmentShaderSource_blendPinLight;
				break;
			case VuoBlendMode_HardMix:
				fragmentShaderSource = fragmentShaderSource_blendHardMix;
				break;
			case VuoBlendMode_Difference:
				fragmentShaderSource = fragmentShaderSource_blendDifference;
				break;
			case VuoBlendMode_Exclusion:
				fragmentShaderSource = fragmentShaderSource_blendExclusion;
				break;
			case VuoBlendMode_Subtract:
				fragmentShaderSource = fragmentShaderSource_blendSubtract;
				break;
			case VuoBlendMode_Divide:
				fragmentShaderSource = fragmentShaderSource_blendDivide;
				break;
			case VuoBlendMode_Hue:
				fragmentShaderSource = fragmentShaderSource_blendHue;
				break;
			case VuoBlendMode_Saturation:
				fragmentShaderSource = fragmentShaderSource_blendSaturation;
				break;
			case VuoBlendMode_Color:
				fragmentShaderSource = fragmentShaderSource_blendColor;
				break;
			case VuoBlendMode_Luminosity:
				fragmentShaderSource = fragmentShaderSource_blendLuminosity;
				break;
		}

		if( (*instance)->currentBlendMode >= 0 )
			VuoRelease((*instance)->shader);

		char *blendModeSummary = VuoBlendMode_getSummary(blendMode);
		(*instance)->shader = VuoShader_make(blendModeSummary);

		free(blendModeSummary);

		VuoShader_addSource((*instance)->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
		VuoRetain((*instance)->shader);
		(*instance)->currentBlendMode = blendMode;
	}

	VuoInteger         pixelsWide = background ? background->pixelsWide             : foreground->pixelsWide;
	VuoInteger         pixelsHigh = background ? background->pixelsHigh             : foreground->pixelsHigh;
	VuoImageColorDepth colorDepth           = VuoImage_getColorDepth(background);
	VuoImageColorDepth colorDepthForeground = VuoImage_getColorDepth(foreground);
	if (colorDepthForeground > colorDepth)
		colorDepth = colorDepthForeground;

	// Associate the input image with the shader.
	VuoShader_setUniform_VuoImage((*instance)->shader, "background", background ? background : VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,0,0,0),1,1));
	VuoShader_setUniform_VuoImage((*instance)->shader, "foreground", foreground ? foreground : VuoImage_makeColorImage(VuoColor_makeWithRGBA(0,0,0,0),1,1));

	VuoShader_setUniform_VuoReal((*instance)->shader, "foregroundOpacity", foregroundOpacity);

	// Render.
	*blended = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, pixelsWide, pixelsHigh, colorDepth);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	if( (*instance)->currentBlendMode >= 0 )
	{
		VuoRelease((*instance)->shader);
		VuoRelease((*instance)->imageRenderer);
	}

	VuoGlContext_disuse((*instance)->glContext);
}
