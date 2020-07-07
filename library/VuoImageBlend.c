/**
 * @file
 * VuoImageBlend implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageBlend.h"
#include "VuoImageRenderer.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoImageBlend",
	"dependencies" : [
		"VuoBlendMode",
		"VuoImageRenderer"
]
});
#endif
/// @}

/// @{
/// Source code for GLSL blend modes.
#define FilterSource "\
	\n#include \"VuoGlslAlpha.glsl\"\
	uniform sampler2D background;\
	uniform sampler2D foreground;\
	uniform float foregroundOpacity;\
	uniform bool replaceOpacity;\
	varying vec2 fragmentTextureCoordinate;\
	\
	float BlendLinearDodgef(float base, float blend) { return min(base + blend, 1.0); }\
	float BlendLinearBurnf(float base, float blend) { return max(base + blend - 1.0, 0.0); }\
	float BlendDividef(float base, float blend) { return (blend == 0.0) ? blend : base/blend; }\
	\
	float BlendLightenf(float base, float blend) { return max(blend, base); }\
	float BlendDarkenf(float base, float blend) { return min(blend, base); }\
	float BlendLinearLightf(float base, float blend) { return (blend < 0.5 ? BlendLinearBurnf(base, (2.0 * blend)) : BlendLinearDodgef(base, (2.0 * (blend - 0.5)))); }\
	\
	float BlendScreenf(float base, float blend) { return (1.0 - ((1.0 - base) * (1.0 - blend))); }\
	float BlendOverlayf(float base, float blend) { return (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend))); }\
	float BlendSoftLightf(float base, float blend) { return ((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend))); }\
	float BlendHardLightf(float base, float blend) { return BlendOverlayf(blend, base); }\
	float BlendColorDodgef(float base, float blend) { return ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0)); }\
	float BlendColorBurnf(float base, float blend) { return (blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0); }\
	\
	float BlendVividLightf(float base, float blend) { return ((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5)))); }\
	float BlendPinLightf(float base, float blend) { return ((blend < 0.5) ? BlendDarkenf(base, (2.0 * blend)) : BlendLightenf(base, (2.0 *(blend - 0.5)))); }\
	float BlendHardMixf(float base, float blend) { return ((BlendVividLightf(base, blend) < 0.5) ? 0.0 : 1.0); }\
	\
	\n#include \"VuoGlslHsl.glsl\"\
	\
	vec3 blendColors(vec4 bg, vec4 fg);\
	\
	void main()                                                                                                                      \
	{                                                                                                                                \
		vec4 backgroundColor = VuoGlsl_sample(background, fragmentTextureCoordinate);                                                \
		backgroundColor.rgb /= backgroundColor.a > 0. ? backgroundColor.a : 1.;                                                      \
		vec4 foregroundColor = VuoGlsl_sample(foreground, fragmentTextureCoordinate);                                                \
		foregroundColor.rgb /= foregroundColor.a > 0. ? foregroundColor.a : 1.;                                                      \
		foregroundColor.a *= foregroundOpacity;                                                                                      \
                                                                                                                                     \
		vec3 blendedColors = clamp(blendColors(backgroundColor, foregroundColor), 0., 1.);                                           \
		if (replaceOpacity)                                                                                                          \
			blendedColors = mix(foregroundColor.rgb, blendedColors, backgroundColor.a);                                              \
                                                                                                                                     \
		vec4 mixed = replaceOpacity                                                                                                  \
			? vec4(blendedColors*foregroundColor.a + backgroundColor.rgb*backgroundColor.a*(1.-foregroundColor.a),                   \
				   foregroundColor.a + backgroundColor.a*(1.-foregroundColor.a))                                                     \
			: vec4(blendedColors*foregroundColor.a*backgroundColor.a + backgroundColor.rgb*backgroundColor.a*(1.-foregroundColor.a), \
				   backgroundColor.a);                                                                                               \
                                                                                                                                     \
		gl_FragColor = mixed;                                                                                                        \
	}                                                                                                                                \
	"

#define BLEND_FILTERS(source) "#version 120\n" FilterSource "\n" #source

static const char * fragmentShaderSource_blendNormal = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return fg.rgb;
	}
);

static const char * fragmentShaderSource_blendMultiply = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb * fg.rgb;
	}
);

static const char * fragmentShaderSource_blendLinearDodge = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb + fg.rgb;
	}
);

static const char * fragmentShaderSource_blendDarkerComponent = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return min(bg.rgb, fg.rgb);
	}
);

static const char * fragmentShaderSource_blendDarkerColor = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		float bgLum = VuoGlsl_rgbToHsl(bg.rgb).b;
		float fgLum = VuoGlsl_rgbToHsl(fg.rgb).b;
		return bgLum < fgLum ? bg.rgb : fg.rgb;
	}
);

static const char * fragmentShaderSource_blendLighterComponent = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return max(bg.rgb, fg.rgb);
	}
);

static const char * fragmentShaderSource_blendLighterColor = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		float bgLum = VuoGlsl_rgbToHsl(bg.rgb).b;
		float fgLum = VuoGlsl_rgbToHsl(fg.rgb).b;
		return bgLum > fgLum ? bg.rgb : fg.rgb;
	}
);

static const char * fragmentShaderSource_blendLinearBurn = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb + fg.rgb - 1.;
	}
);

static const char * fragmentShaderSource_blendColorBurn = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendColorBurnf(bg.r, fg.r),
					BlendColorBurnf(bg.g, fg.g),
					BlendColorBurnf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendColorDodge = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb / (1. - fg.rgb*fg.a);
	}
);

static const char * fragmentShaderSource_blendScreen = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendScreenf(bg.r, fg.r),
					BlendScreenf(bg.g, fg.g),
					BlendScreenf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendOverlay = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendOverlayf(bg.r, fg.r),
					BlendOverlayf(bg.g, fg.g),
					BlendOverlayf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendSoftLight = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendSoftLightf(bg.r, fg.r),
					BlendSoftLightf(bg.g, fg.g),
					BlendSoftLightf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendHardLight = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendHardLightf(bg.r, fg.r),
					BlendHardLightf(bg.g, fg.g),
					BlendHardLightf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendVividLight = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendVividLightf(bg.r, fg.r),
					BlendVividLightf(bg.g, fg.g),
					BlendVividLightf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendLinearLight = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendLinearLightf(bg.r, fg.r),
					BlendLinearLightf(bg.g, fg.g),
					BlendLinearLightf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendPinLight = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendPinLightf(bg.r, fg.r),
					BlendPinLightf(bg.g, fg.g),
					BlendPinLightf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendHardMix = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendHardMixf(bg.r, fg.r),
					BlendHardMixf(bg.g, fg.g),
					BlendHardMixf(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendDifference = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return abs(bg.rgb - fg.rgb);
	}
);

static const char * fragmentShaderSource_blendExclusion = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb + fg.rgb - 2.*bg.rgb*fg.rgb;
	}
);

static const char * fragmentShaderSource_blendSubtract = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return bg.rgb - fg.rgb;
	}
);

static const char * fragmentShaderSource_blendDivide = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return vec3(BlendDividef(bg.r, fg.r),
					BlendDividef(bg.g, fg.g),
					BlendDividef(bg.b, fg.b));
	}
);

static const char * fragmentShaderSource_blendHue = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		vec3 baseHSL = VuoGlsl_rgbToHsl(bg.rgb);
		return VuoGlsl_hslToRgb(vec3(VuoGlsl_rgbToHsl(fg.rgb).r, baseHSL.g, baseHSL.b));
	}
);

static const char * fragmentShaderSource_blendSaturation = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		vec3 baseHSL = VuoGlsl_rgbToHsl(bg.rgb);
		return VuoGlsl_hslToRgb(vec3(baseHSL.r, VuoGlsl_rgbToHsl(fg.rgb).g, baseHSL.b));
	}
);

static const char * fragmentShaderSource_blendColor = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		vec3 blendHSL = VuoGlsl_rgbToHsl(fg.rgb);
		return VuoGlsl_hslToRgb(vec3(blendHSL.r, blendHSL.g, VuoGlsl_rgbToHsl(bg.rgb).b));
	}
);

static const char * fragmentShaderSource_blendLuminosity = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		vec3 baseHSL = VuoGlsl_rgbToHsl(bg.rgb);
		return VuoGlsl_hslToRgb(vec3(baseHSL.r, baseHSL.g, VuoGlsl_rgbToHsl(fg.rgb).b));
	}
);

static const char * fragmentShaderSource_blendPower = BLEND_FILTERS(
	vec3 blendColors(vec4 bg, vec4 fg)
	{
		return pow(bg.rgb, fg.rgb);
	}
);
/// @}

/**
 * Creates state data for the image Blendr.
 */
VuoImageBlend VuoImageBlend_make(VuoBlendMode blendMode)
{
	const char *fragmentShaderSource = fragmentShaderSource_blendNormal;
	switch (blendMode)
	{
		case VuoBlendMode_Normal:
			fragmentShaderSource = fragmentShaderSource_blendNormal;
			break;
		case VuoBlendMode_Multiply:
			fragmentShaderSource = fragmentShaderSource_blendMultiply;
			break;
		case VuoBlendMode_DarkerComponents:
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
		case VuoBlendMode_LighterComponents:
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
		case VuoBlendMode_Power:
			fragmentShaderSource = fragmentShaderSource_blendPower;
			break;
	}

	char *blendModeSummary = VuoBlendMode_getSummary(blendMode);
	VuoShader shader = VuoShader_make(blendModeSummary);
	free(blendModeSummary);

	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);

	return (VuoImageBlend)shader;
}

/**
 * Blends multiple images together into a single image.
 */
VuoImage VuoImageBlend_blend(VuoImageBlend blend, VuoImage background, VuoImage foreground, VuoReal foregroundOpacity, VuoBoolean replaceOpacity)
{
	if (!background && !foreground)
		return NULL;

	VuoShader shader = (VuoShader)blend;

	VuoInteger         pixelsWide = background ? background->pixelsWide             : foreground->pixelsWide;
	VuoInteger         pixelsHigh = background ? background->pixelsHigh             : foreground->pixelsHigh;
	VuoImageColorDepth colorDepth           = VuoImage_getColorDepth(background);
	VuoImageColorDepth colorDepthForeground = VuoImage_getColorDepth(foreground);
	if (colorDepthForeground > colorDepth)
		colorDepth = colorDepthForeground;

	if (!background)
	{
		background = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0, 0, 0, 0), 1, 1);
		background->scaleFactor = foreground->scaleFactor;
	}
	VuoShader_setUniform_VuoImage(shader, "background", background);

	if (!foreground)
	{
		foreground = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0, 0, 0, 0), 1, 1);
		foreground->scaleFactor = background->scaleFactor;
	}
	VuoShader_setUniform_VuoImage(shader, "foreground", foreground);

	VuoShader_setUniform_VuoReal(shader, "foregroundOpacity", foregroundOpacity);
	VuoShader_setUniform_VuoBoolean(shader, "replaceOpacity", replaceOpacity);

	return VuoImageRenderer_render(shader, pixelsWide, pixelsHigh, colorDepth);
}
