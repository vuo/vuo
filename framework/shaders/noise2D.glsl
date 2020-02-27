/**
 * Classic Perlin noise.
 * This version makes heavy use of a texture for table lookups.
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 *
 * You may use, modify and redistribute this code free of charge,
 * provided that the author's names and this notice appear intact.
 */

uniform sampler2D perlinTexture;

/**
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another texture size.

/**
 * The interpolation function. This could be a 1D texture lookup
 * to get some more speed, but it's not the main part of the algorithm.
 */
vec2 fade(vec2 t) {
	// return t*t*(3.0-2.0*t); // Old fade, yields discontinuous second derivative
	return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/**
 * 2D classic Perlin noise. Fast, but less useful than 3D noise.
 */
float snoise2D1D(vec2 P)
{
	vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
	vec2 Pf = fract(P);             // Fractional part for interpolation

	// Noise contribution from lower left corner
	vec2 grad00 = texture2D(perlinTexture, Pi).rg * 4.0 - 1.0;
	float n00 = dot(grad00, Pf);

	// Noise contribution from lower right corner
	vec2 grad10 = texture2D(perlinTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
	float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

	// Noise contribution from upper left corner
	vec2 grad01 = texture2D(perlinTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
	float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

	// Noise contribution from upper right corner
	vec2 grad11 = texture2D(perlinTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
	float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

	vec2 fade_xy = fade(Pf);
	// Blend contributions along x
	vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);

	// Blend contributions along y
	float n_xy = mix(n_x.x, n_x.y, fade_xy.y);

	// We're done, return the final noise value.
	return n_xy;
}

vec2 snoise2D2D(vec2 v)
{
	return vec2(snoise2D1D(vec2( v.x,  v.y)),
				snoise2D1D(vec2(-v.x, -v.y)));
}

vec3 snoise2D3D(vec2 v)
{
	return vec3(snoise2D1D(vec2(v.x-1, v.y-1)),
				snoise2D1D(vec2(v.x,   v.y)),
				snoise2D1D(vec2(v.x+1, v.y+1)));
}

vec4 snoise2D4D(vec2 v)
{
	return vec4(snoise2D1D(vec2(v.x-1, v.y-1)),
				snoise2D1D(vec2(v.x,   v.y)),
				snoise2D1D(vec2(v.x+1, v.y+1)),
				snoise2D1D(vec2(v.x+2, v.y+2)));
}
