/**
 * Functions for handling alpha transparency.
 */

/**
 * Samples a color from the specified texture,
 * and ensures its alpha is between 0 and 1 inclusive.
 *
 * The returned RGB values are premultiplied by alpha.
 */
vec4 VuoGlsl_sample(sampler2D texture, vec2 position)
{
	vec4 color = texture2D(texture, position);
	color.a = clamp(color.a, 0., 1.);	// floating-point textures can have alphas outside the 0–1 range.
	return color;
}

/**
 * Samples a color from the specified texture,
 * and ensures its alpha is between 0 and 1 inclusive.
 *
 * The returned RGB values are premultiplied by alpha.
 */
vec4 VuoGlsl_sampleRect(sampler2DRect texture, vec2 position)
{
	vec4 color = texture2DRect(texture, position);
	color.a = clamp(color.a, 0., 1.);	// floating-point textures can have alphas outside the 0–1 range.
	return color;
}

/**
 * Discards the fragment if the alpha is so small as to be invisible.
 */
void VuoGlsl_discardInvisible(float alpha)
{
	if (alpha < 1./65535.)
		discard;
}
