/**
 * @file
 * Functions for evaluating the brightness of a color.
 */

/**
 * Returns a measure of the brightness of `color`.
 *
 * @param color A straight (un-premultiplied) color.
 * @param type @ref VuoThresholdType
 *
 * @see VuoColor_brightness
 */
float VuoGlsl_brightness(vec4 color, int type)
{
	if (type == 0) // VuoThresholdType_Rec601
		return dot(color.rgb, vec3(.299, .587, .114));
	else if (type == 1) // VuoThresholdType_Rec709
		return pow(dot(pow(color.rgb, vec3(2.2)), vec3(.2126, .7152, .0722)), 1./2.2);
	else if (type == 2) // VuoThresholdType_Desaturate
		return (max(color.r, max(color.g, color.b)) + min(color.r, min(color.g, color.b))) / 2.;
	else if (type == 3) // VuoThresholdType_RGBAverage
		return (color.r + color.g + color.b) / 3.;
	else if (type == 4) // VuoThresholdType_RGBMaximum
		return max(color.r, max(color.g, color.b));
	else if (type == 5) // VuoThresholdType_RGBMinimum
		return min(color.r, min(color.g, color.b));
	else if (type == 6) // VuoThresholdType_Red
		return color.r;
	else if (type == 7) // VuoThresholdType_Green
		return color.g;
	else if (type == 8) // VuoThresholdType_Blue
		return color.b;
	else if (type == 9) // VuoThresholdType_Alpha
		return color.a;
}

/**
 * Converts `color` to grayscale (or, if type is VuoThresholdType_RGB, leaves it as-is).
 *
 * @param color A straight (un-premultiplied) color.
 * @param type @ref VuoThresholdType
 * @return A premultiplied color.
 */
vec4 VuoGlsl_gray(vec4 color, int type)
{
	if (type == 10) // VuoThresholdType_RGB
		return color;
	else
		return vec4(vec3(VuoGlsl_brightness(color, type)), 1) * color.a;
}
