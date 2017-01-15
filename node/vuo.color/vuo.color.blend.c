/**
 * @file
 * vuo.color.blend node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoBlendMode.h"

VuoModuleMetadata({
					 "title" : "Blend Colors",
					 "keywords" : [ "combine", "mix", "fade", "merge", "layer", "composite", "channel",
						"normal", "add", "additive", "alpha", "opacity", "transparent", "transparency",
						"multiply", "darker", "linear burn", "color burn", "burn",
						"screen", "lighter", "linear dodge", "color dodge", "dodge",
						"overlay", "soft light", "hard light", "vivid light", "linear light", "pin light", "light", "hard mix",
						"difference", "exclusion", "subtract", "divide",
						"hue", "saturation", "color", "luminosity",
						"tint", "tone", "chroma" ],
					 "version" : "1.0.0",
					 "dependencies" : [
					 ],
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

static float lerp(float a, float b, float t)
{
	return (1-t)*a + t*b;
}

static float min(float a, float b)
{
	return a < b ? a : b;
}

static float max(float a, float b)
{
	return a > b ? a : b;
}

static float clamp(float val, float min, float max)
{
	if(val > max)
		return max;
	else if(val < min)
		return min;
	else
		return val;
}

static VuoPoint3d getRGB(VuoColor col)
{
	return (VuoPoint3d){ col.r, col.g, col.b };
}

static VuoColor colorWithRgbAndA(VuoPoint3d rgb, float a)
{
	return VuoColor_makeWithRGBA(rgb.x, rgb.y, rgb.z, a);
}

static float BlendLinearDodgef(float base, float blend) { return min(base + blend, 1.0); }
static float BlendDarkenf(float base, float blend) { return min(blend, base); }
static float BlendLightenf(float base, float blend) { return max(blend, base); }
static float BlendLinearBurnf(float base, float blend) { return max(base + blend - 1.0, 0.0); }
static float BlendColorBurnf(float base, float blend) { return (blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0); }
static float BlendColorDodgef(float base, float blend) { return ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0)); }
static float BlendLinearLightf(float base, float blend) { return (blend < 0.5 ? BlendLinearBurnf(base, (2.0 * blend)) : BlendLinearDodgef(base, (2.0 * (blend - 0.5)))); }
static float BlendPinLightf(float base, float blend) { return ((blend < 0.5) ? BlendDarkenf(base, (2.0 * blend)) : BlendLightenf(base, (2.0 *(blend - 0.5)))); }
static float BlendVividLightf(float base, float blend) { return ((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5)))); }
static float BlendHardMixf(float base, float blend) { return ((BlendVividLightf(base, blend) < 0.5) ? 0.0 : 1.0); }

static float HueToRGB(float f1, float f2, float hue)
{
	if (hue < 0.0)
		hue += 1.0;
	else if (hue > 1.0)
		hue -= 1.0;
	float res;
	if ((6.0 * hue) < 1.0)
		res = f1 + (f2 - f1) * 6.0 * hue;
	else if ((2.0 * hue) < 1.0)
		res = f2;
	else if ((3.0 * hue) < 2.0)
		res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
	else
		res = f1;
	return res;
}

static VuoPoint3d HSLToRGB(VuoPoint3d hsl)
{
	VuoPoint3d rgb;

	if (hsl.y == 0.0)
		rgb = (VuoPoint3d){ hsl.z, hsl.z, hsl.z };
	else
	{
		float f2;

		if (hsl.z < 0.5)
			f2 = hsl.z * (1.0 + hsl.y);
		else
			f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);

		float f1 = 2.0 * hsl.z - f2;

		rgb.x = HueToRGB(f1, f2, hsl.x + (1.0/3.0));
		rgb.y = HueToRGB(f1, f2, hsl.x);
		rgb.z= HueToRGB(f1, f2, hsl.x - (1.0/3.0));
	}

	return rgb;
}

static VuoColor blendNormal(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d mixed = VuoPoint3d_lerp(baseRGB, blendRGB, blend.a);
	VuoPoint3d result = VuoPoint3d_lerp(baseRGB, mixed, foregroundOpacity);

	return colorWithRgbAndA(result, base.a + blend.a * foregroundOpacity);
}

static VuoColor blendMultiply(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = VuoPoint3d_scale(baseRGB, blendRGB);

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	result = VuoPoint3d_lerp(baseRGB, result, foregroundOpacity);

	return	colorWithRgbAndA(result, base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLinearDodge(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { BlendLinearDodgef(base.r, blend.r),
						  BlendLinearDodgef(base.g, blend.g),
						  BlendLinearDodgef(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity );
}

static VuoColor blendDarkerComponent(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { min(base.r, blend.r), min(base.g, blend.g), min(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendDarkerColor(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	float base_lum = l;
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	float blend_lum = l;

	VuoPoint3d result = base_lum < blend_lum ? baseRGB : blendRGB;

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLinearBurn(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { base.r + blend.r - 1.,
						  base.g + blend.g - 1.,
						  base.b + blend.b - 1. };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendColorBurn(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	// 1-(1-b)/f
	VuoPoint3d result ={1-min((1-base.r)/blend.r,1.),
						1-min((1-base.g)/blend.g,1.),
						1-min((1-base.b)/blend.b,1.)};

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendScreen(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	// 1-(1-b)•(1-f)
	VuoPoint3d result = { 	1-(1-base.r) * (1-blend.r),
							1-(1-base.g) * (1-blend.g),
							1-(1-base.b) * (1-blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLighterComponent(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { max(base.r, blend.r), max(base.g, blend.g), max(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLighterColor(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	float base_lum = l;
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	float blend_lum = l;

	VuoPoint3d result = base_lum > blend_lum ? baseRGB : blendRGB;

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendColorDodge(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	// b/(1-f)
	VuoPoint3d result = {	clamp( base.r / (1-blend.r), 0., 1. ),
							clamp( base.g / (1-blend.g), 0., 1. ),
							clamp( base.b / (1-blend.b), 0., 1. ) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendOverlay(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	// (base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend)))
	VuoPoint3d result = { (base.r < 0.5 ? (2.0 * base.r * blend.r) : (1.0 - 2.0 * (1.0 - base.r) * (1.0 - blend.r))),
						  (base.g < 0.5 ? (2.0 * base.g * blend.g) : (1.0 - 2.0 * (1.0 - base.g) * (1.0 - blend.g))),
						  (base.b < 0.5 ? (2.0 * base.b * blend.b) : (1.0 - 2.0 * (1.0 - base.b) * (1.0 - blend.b))) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendSoftLight(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	((blend.r < 0.5) ? (2.0 * base.r * blend.r + base.r * base.r * (1.0 - 2.0 * blend.r)) : (sqrt(base.r) * (2.0 * blend.r - 1.0) + 2.0 * base.r * (1.0 - blend.r))),
							((blend.g < 0.5) ? (2.0 * base.g * blend.g + base.g * base.g * (1.0 - 2.0 * blend.g)) : (sqrt(base.g) * (2.0 * blend.g - 1.0) + 2.0 * base.g * (1.0 - blend.g))),
							((blend.b < 0.5) ? (2.0 * base.b * blend.b + base.b * base.b * (1.0 - 2.0 * blend.b)) : (sqrt(base.b) * (2.0 * blend.b - 1.0) + 2.0 * base.b * (1.0 - blend.b))) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendHardLight(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	(blend.r < 0.5 ? (2.0 * blend.r * base.r) : (1.0 - 2.0 * (1.0 - blend.r) * (1.0 - base.r))),
							(blend.g < 0.5 ? (2.0 * blend.g * base.g) : (1.0 - 2.0 * (1.0 - blend.g) * (1.0 - base.g))),
							(blend.b < 0.5 ? (2.0 * blend.b * base.b) : (1.0 - 2.0 * (1.0 - blend.b) * (1.0 - base.b))) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendVividLight(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { ((blend.r < 0.5) ? BlendColorBurnf(base.r, (2.0 * blend.r)) : BlendColorDodgef(base.r, (2.0 * (blend.r - 0.5)))),
						  ((blend.g < 0.5) ? BlendColorBurnf(base.g, (2.0 * blend.g)) : BlendColorDodgef(base.g, (2.0 * (blend.g - 0.5)))),
						  ((blend.b < 0.5) ? BlendColorBurnf(base.b, (2.0 * blend.b)) : BlendColorDodgef(base.b, (2.0 * (blend.b - 0.5)))) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLinearLight(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = {
					BlendLinearLightf(base.r, blend.r),
					BlendLinearLightf(base.g, blend.g),
					BlendLinearLightf(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendPinLight(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	BlendPinLightf(base.r, blend.r),
							BlendPinLightf(base.g, blend.g),
							BlendPinLightf(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendHardMix(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	BlendHardMixf(base.r, blend.r),
							BlendHardMixf(base.g, blend.g),
							BlendHardMixf(base.b, blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendDifference(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	fabs(base.r - blend.r),
							fabs(base.g - blend.g),
							fabs(base.b - blend.b) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendExclusion(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	// b+f-2•b•f
	VuoPoint3d result = { 	base.r+blend.r - 2*base.r*blend.r,
							base.g+blend.g - 2*base.g*blend.g,
							base.b+blend.b - 2*base.b*blend.b };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendSubtract(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoPoint3d result = { 	clamp(base.r - blend.r, 0., 1.),
							clamp(base.g - blend.g, 0., 1.),
							clamp(base.b - blend.b, 0., 1.) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendDivide(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);


	VuoPoint3d result = {	clamp(base.r / blend.r, 0., 1.),
							clamp(base.g / blend.g, 0., 1.),
							clamp(base.b / blend.b, 0., 1.) };

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendHue(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	float baseSat = s;
	float baseLum = l;
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	float blendHue = h;

	VuoPoint3d result = HSLToRGB( (VuoPoint3d){ blendHue, baseSat, baseLum } );

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendSaturation(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	VuoPoint3d baseHSL = { h, s, l };
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	VuoPoint3d blendHSL = { h, s, l };

	VuoPoint3d result = HSLToRGB( (VuoPoint3d){baseHSL.x, blendHSL.y, baseHSL.z});

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendColor(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	VuoPoint3d baseHSL = { h, s, l };
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	VuoPoint3d blendHSL = { h, s, l };

	VuoPoint3d result = HSLToRGB( (VuoPoint3d){blendHSL.x, blendHSL.y, baseHSL.z});

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}

static VuoColor blendLuminosity(VuoColor base, VuoColor blend, float foregroundOpacity)
{
	VuoPoint3d baseRGB = getRGB(base);
	VuoPoint3d blendRGB = getRGB(blend);

	VuoReal h, s, l, a;
	VuoColor_getHSLA(base, &h, &s, &l, &a);
	VuoPoint3d baseHSL = { h, s, l };
	VuoColor_getHSLA(blend, &h, &s, &l, &a);
	VuoPoint3d blendHSL = { h, s, l };

	VuoPoint3d result = HSLToRGB((VuoPoint3d){baseHSL.x, baseHSL.y, blendHSL.z});

	result = VuoPoint3d_lerp(baseRGB, result, blend.a);
	result = VuoPoint3d_lerp(blendRGB, result, base.a);

	return colorWithRgbAndA( VuoPoint3d_lerp(baseRGB, result, foregroundOpacity), base.a + blend.a * foregroundOpacity);
}


void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) background,
		VuoInputData(VuoColor, {"default":{"r":0,"g":0,"b":0,"a":1}}) foreground,
		VuoInputData(VuoBlendMode, {"default":"normal"}) blendMode,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0, "suggestedMax":1}) foregroundOpacity,
		VuoOutputData(VuoColor) blendedColor
)
{
	switch(blendMode)
	{
		case VuoBlendMode_Normal:
			*blendedColor = blendNormal(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Multiply:
			*blendedColor = blendMultiply(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_DarkerComponent:
			*blendedColor = blendDarkerComponent(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_DarkerColor:
			*blendedColor = blendDarkerColor(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_LinearBurn:
			*blendedColor = blendLinearBurn(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_ColorBurn:
			*blendedColor = blendColorBurn(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Screen:
			*blendedColor = blendScreen(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_LighterComponent:
			*blendedColor = blendLighterComponent(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_LighterColor:
			*blendedColor = blendLighterColor(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_LinearDodge:
			*blendedColor = blendLinearDodge(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_ColorDodge:
			*blendedColor = blendColorDodge(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Overlay:
			*blendedColor = blendOverlay(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_SoftLight:
			*blendedColor = blendSoftLight(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_HardLight:
			*blendedColor = blendHardLight(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_VividLight:
			*blendedColor = blendVividLight(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_LinearLight:
			*blendedColor = blendLinearLight(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_PinLight:
			*blendedColor = blendPinLight(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_HardMix:
			*blendedColor = blendHardMix(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Difference:
			*blendedColor = blendDifference(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Exclusion:
			*blendedColor = blendExclusion(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Subtract:
			*blendedColor = blendSubtract(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Divide:
			*blendedColor = blendDivide(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Hue:
			*blendedColor = blendHue(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Saturation:
			*blendedColor = blendSaturation(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Color:
			*blendedColor = blendColor(background, foreground, foregroundOpacity);
			break;
		case VuoBlendMode_Luminosity:
			*blendedColor = blendLuminosity(background, foreground, foregroundOpacity);
			break;
	}
}
