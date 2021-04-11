/**
 * @file
 * VuoColor implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"

#include "VuoThresholdType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Color",
					 "description" : "A color, with component values specifying red, green, blue, and alpha (opacity).",
					 "keywords" : [ "rgba" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoColor",
						"VuoReal",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoColor
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "r" : 0.5,
 *     "g" : 1,
 *     "b" : 0,
 *     "a" : 1
 *   }
 * }
 *
 * @version200Changed{The alpha channel now defaults to 1 (previously it defaulted to 0).}
 */
VuoColor VuoColor_makeFromJson(json_object * js)
{
	VuoColor color = {0,0,0,1};

	json_type t = json_object_get_type(js);
	if (t == json_type_string)
	{
		const char *s = json_object_get_string(js);
		if (s[0] == '#')
		{
			size_t len = strlen(s);
			if (len == 4)	// "#rgb"
			{
				long r = VuoInteger_makeFromHexByte(s[1]);
				long g = VuoInteger_makeFromHexByte(s[2]);
				long b = VuoInteger_makeFromHexByte(s[3]);
				color.r = (float)r / 15;
				color.g = (float)g / 15;
				color.b = (float)b / 15;
				color.a = 1;
			}
			else if (len == 5)	// "#rgba"
			{
				long r = VuoInteger_makeFromHexByte(s[1]);
				long g = VuoInteger_makeFromHexByte(s[2]);
				long b = VuoInteger_makeFromHexByte(s[3]);
				long a = VuoInteger_makeFromHexByte(s[4]);
				color.r = (float)r / 15;
				color.g = (float)g / 15;
				color.b = (float)b / 15;
				color.a = (float)a / 15;
			}
			else if (len == 7)	// "#rrggbb"
			{
				long r = (VuoInteger_makeFromHexByte(s[1]) << 4) + VuoInteger_makeFromHexByte(s[2]);
				long g = (VuoInteger_makeFromHexByte(s[3]) << 4) + VuoInteger_makeFromHexByte(s[4]);
				long b = (VuoInteger_makeFromHexByte(s[5]) << 4) + VuoInteger_makeFromHexByte(s[6]);
				color.r = (float)r / 255;
				color.g = (float)g / 255;
				color.b = (float)b / 255;
				color.a = 1;
			}
			else if (len == 9)	// "#rrggbbaa"
			{
				long r = (VuoInteger_makeFromHexByte(s[1]) << 4) + VuoInteger_makeFromHexByte(s[2]);
				long g = (VuoInteger_makeFromHexByte(s[3]) << 4) + VuoInteger_makeFromHexByte(s[4]);
				long b = (VuoInteger_makeFromHexByte(s[5]) << 4) + VuoInteger_makeFromHexByte(s[6]);
				long a = (VuoInteger_makeFromHexByte(s[7]) << 4) + VuoInteger_makeFromHexByte(s[8]);
				color.r = (float)r / 255;
				color.g = (float)g / 255;
				color.b = (float)b / 255;
				color.a = (float)a / 255;
			}
		}
		else
		{
			// "r,g,b" or "r,g,b,a"
			color.a = 1;
			sscanf(s, "%20g, %20g, %20g, %20g", &color.r, &color.g, &color.b, &color.a);
		}
		return color;
	}
	else if (t == json_type_array)
	{
		int len = json_object_array_length(js);
		if (len >= 1)
			color.r = json_object_get_double(json_object_array_get_idx(js, 0));
		if (len >= 2)
			color.g = json_object_get_double(json_object_array_get_idx(js, 1));
		if (len >= 3)
			color.b = json_object_get_double(json_object_array_get_idx(js, 2));
		if (len >= 4)
			color.a = json_object_get_double(json_object_array_get_idx(js, 3));
		else
			color.a = 1;
	}

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "r", &o))
		color.r = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "g", &o))
		color.g = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "b", &o))
		color.b = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "a", &o))
		color.a = VuoReal_makeFromJson(o);

	return color;
}

/**
 * @ingroup VuoColor
 * Encodes @c value as a JSON object.
 */
json_object * VuoColor_getJson(const VuoColor value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "r", VuoReal_getJson(value.r));

	json_object_object_add(js, "g", VuoReal_getJson(value.g));

	json_object_object_add(js, "b", VuoReal_getJson(value.b));

	json_object_object_add(js, "a", VuoReal_getJson(value.a));

	return js;
}

/**
 * Returns a short HTML representation of `value`.
 */
char *VuoColor_getShortSummary(const VuoColor value)
{
	return VuoText_format("<span style='background-color:#%02x%02x%02x;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> %4.02f, %4.02f, %4.02f, %4.02f",
						  (unsigned int)(VuoReal_clamp(value.r,0,1) * 255),
						  (unsigned int)(VuoReal_clamp(value.g,0,1) * 255),
						  (unsigned int)(VuoReal_clamp(value.b,0,1) * 255),
						  value.r, value.g, value.b, value.a);
}

/**
 * Returns a detailed HTML representation of `value`.
 */
char *VuoColor_getSummary(const VuoColor value)
{
	const char *t = VUO_STRINGIFY(
		<style>
			th,td { text-align: right; }
			td { font-weight: normal; }
			.left { text-align: left; }
		</style>
		<table cellspacing=6>
			<tr>
				<td class='left'><span style='background-color:%s;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span></td>
				<th>Red</th>
				<th>Green</th>
				<th>Blue</th>
				<th>Alpha</th>
			</tr>\n
			<tr>
				<th>Normalized:</th>
				<td>%4.03f</td>
				<td>%4.03f</td>
				<td>%4.03f</td>
				<td>%4.03f</td>
			</tr>\n
			<tr>
				<th>8-bit:</th>
				<td>%d</td>
				<td>%d</td>
				<td>%d</td>
				<td>%d</td>
			</tr>\n
			<tr>
				<th>Hex:</th>
				<td class='left' colspan=4>&nbsp;%s</td>
			</tr>
		</table>
	);

	VuoText hex = VuoColor_getHex(value, false);
	VuoLocal(hex);
	return VuoText_format(t,
						  hex,
						  value.r, value.g, value.b, value.a,
						  (unsigned int)(VuoReal_clamp(value.r,0,1) * 255),
						  (unsigned int)(VuoReal_clamp(value.g,0,1) * 255),
						  (unsigned int)(VuoReal_clamp(value.b,0,1) * 255),
						  (unsigned int)(VuoReal_clamp(value.a,0,1) * 255),
						  hex
						  );
}

/**
 * Returns a hex string (like `#445566ff`) representing the color.
 */
VuoText VuoColor_getHex(VuoColor color, VuoBoolean includeAlpha)
{
	if (includeAlpha)
		return VuoText_make(VuoText_format("#%02x%02x%02x%02x",
										   (unsigned int)(VuoReal_clamp(color.r,0,1) * 255),
										   (unsigned int)(VuoReal_clamp(color.g,0,1) * 255),
										   (unsigned int)(VuoReal_clamp(color.b,0,1) * 255),
										   (unsigned int)(VuoReal_clamp(color.a,0,1) * 255)
										   ));
	else
		return VuoText_make(VuoText_format("#%02x%02x%02x",
										   (unsigned int)(VuoReal_clamp(color.r,0,1) * 255),
										   (unsigned int)(VuoReal_clamp(color.g,0,1) * 255),
										   (unsigned int)(VuoReal_clamp(color.b,0,1) * 255)
										   ));
}

/**
 * Returns a @c VuoColor with the given hue, saturation, lightness, alpha.
 *
 * @param hue Color circle from 0 (red) to 1/3 (green) to 2/3 (blue) to 1 (red).  Values beyond that range are wrapped.
 * @param saturation 0 to 1
 * @param luminosity 0 to 1
 * @param alpha 0 to 1
 */
VuoColor VuoColor_makeWithHSLA(VuoReal hue, VuoReal saturation, VuoReal luminosity, VuoReal alpha)
{
	// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c

	float r, g, b;

	if (saturation < 0.00001)
		r = g = b = luminosity;
	else
	{
		VuoReal hueWrapped = fmod(hue, 1);
		float (^hue2rgb)(float p, float q, float t) = ^(float p, float q, float t) {
			if (t < 0.f) t += 1.f;
			if (t > 1.f) t -= 1.f;
			if (t < 1.f/6.f) return p + (q - p) * 6.f * t;
			if (t < 1.f/2.f) return q;
			if (t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
			return p;
		};

		float l = VuoReal_clamp(luminosity, 0, 1);
		float s = VuoReal_clamp(saturation, 0, 1);
		float q = luminosity < 0.5f ? l * (1.f + s) : l + s - l * s;
		float p = 2.f * l - q;
		r = hue2rgb(p, q, hueWrapped + 1.f/3.f);
		g = hue2rgb(p, q, hueWrapped);
		b = hue2rgb(p, q, hueWrapped - 1.f/3.f);
	}

	return VuoColor_makeWithRGBA(r, g, b, alpha);
}

/**
 * Gets the hue, saturation, lightness, hue, alpha of a color.
 *
 * Each value is in the range [0,1].
 */
void VuoColor_getHSLA(VuoColor color, VuoReal *h, VuoReal *s, VuoReal *l, VuoReal *a)
{
	// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c

	VuoReal r, g, b;
	VuoColor_getRGBA(color, &r, &g, &b, a);

	float max = fmax(r, fmax(g, b));
	float min = fmin(r, fmin(g, b));
	*h = *s = *l = (max + min) / 2.f;

	if (max == min)
	{
		*h = *s = 0;
	}
	else
	{
		float d = max - min;
		*s = *l > 0.5f ? d / (2.f - max - min) : d / (max + min);
		if (max == r)
			*h = (g - b) / d + (g < b ? 6.f : 0.f);
		else if (max == g)
			*h = (b - r) / d + 2.f;
		else
			*h = (r - g) / d + 4.f;
		*h /= 6.f;
	}
}

/**
 * Returns the HSL lightness of the color, in the range [0,1].
 */
VuoReal VuoColor_getLightness(VuoColor color)
{
	float max = fmax(color.r, fmax(color.g, color.b));
	float min = fmin(color.r, fmin(color.g, color.b));
	return (max + min) / 2.;
}

/**
 * Returns the weighted (by alpha) average of @c colors.
 *
 * If there are no colors in the list, returns transparent black.
 */
VuoColor VuoColor_average(VuoList_VuoColor colors)
{
	VuoColor result = VuoColor_makeWithRGBA(0,0,0,0);
	unsigned long colorCount = VuoListGetCount_VuoColor(colors);
	for (unsigned long i = 1; i <= colorCount; ++i)
	{
		VuoColor color = VuoListGetValue_VuoColor(colors, i);
		result.r += color.r * color.a;
		result.g += color.g * color.a;
		result.b += color.b * color.a;
		result.a += color.a;
	}

	if (result.a < 0.00001)
		return VuoColor_makeWithRGBA(0,0,0,0);

	result.r /= result.a;
	result.g /= result.a;
	result.b /= result.a;
	result.a /= colorCount;
	return result;
}

/**
 * Returns true if all colors are fully opaque.
 *
 * If the list is empty, returns false.
 *
 * @version200New
 */
bool VuoColor_areAllOpaque(VuoList_VuoColor colors)
{
	size_t itemCount = VuoListGetCount_VuoColor(colors);
	if (itemCount == 0)
		return false;

	VuoColor *itemData = VuoListGetData_VuoColor(colors);
	for (size_t i = 0; i < itemCount; ++i)
		if (!VuoColor_isOpaque(itemData[i]))
			return false;

	return true;
}

/**
 * Returns a measure of the brightness of `color`.
 *
 * @see VuoGlsl_brightness
 *
 * @version200New
 */
VuoReal VuoColor_brightness(VuoColor color, int32_t type)
{
	VuoColor c = VuoColor_premultiply(color);
	if (type == VuoThresholdType_Rec601)
		return c.r * .299 + c.g * .587 + c.b * .114;
	else if (type == VuoThresholdType_Rec709)
		return pow(
			  pow(c.r, 2.2) * .2126
			+ pow(c.g, 2.2) * .7152
			+ pow(c.b, 2.2) * .0722,
			1./2.2);
	else if (type == VuoThresholdType_Desaturate)
		return (MAX(c.r, MAX(c.g, c.b)) + MIN(c.r, MIN(c.g, c.b))) / 2.;
	else if (type == VuoThresholdType_RGBAverage)
		return (c.r + c.g + c.b) / 3.;
	else if (type == VuoThresholdType_RGBMaximum)
		return MAX(c.r, MAX(c.g, c.b));
	else if (type == VuoThresholdType_RGBMinimum)
		return MIN(c.r, MIN(c.g, c.b));
	else if (type == VuoThresholdType_Red)
		return c.r;
	else if (type == VuoThresholdType_Green)
		return c.g;
	else if (type == VuoThresholdType_Blue)
		return c.b;
	else // if (type == VuoThresholdType_Alpha)
		return c.a;
}

/**
 * Returns true if both colors have the same intensity and alpha values.
 */
bool VuoColor_areEqual(const VuoColor value1, const VuoColor value2)
{
	return VuoReal_areEqual(value1.r, value2.r)
		&& VuoReal_areEqual(value1.g, value2.g)
		&& VuoReal_areEqual(value1.b, value2.b)
		&& VuoReal_areEqual(value1.a, value2.a);
}

/**
 * Like @ref VuoColor_areEqual(), but permits color channel values to differ by up to `tolerance`.
 */
bool VuoColor_areEqualWithinTolerance(const VuoColor a, const VuoColor b, const float tolerance)
{
	return fabs(a.r - b.r) <= tolerance
		&& fabs(a.g - b.g) <= tolerance
		&& fabs(a.b - b.b) <= tolerance
		&& fabs(a.a - b.a) <= tolerance;
}

/**
 * Returns true if a < b.
 */
bool VuoColor_isLessThan(const VuoColor a, const VuoColor b)
{
	if (a.r < b.r) return true;
	if (a.r > b.r) return false;

	if (a.g < b.g) return true;
	if (a.g > b.g) return false;

	if (a.b < b.b) return true;
	if (a.b > b.b) return false;

	if (a.a < b.a) return true;
//	if (a.a > b.a) return false;

	return false;
}
