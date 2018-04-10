/**
 * Simplex Perlin noise.
 * This version makes heavy use of a texture for table lookups.
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 * Simplex indexing functions by Bill Licea-Kane, ATI (bill@ati.com)
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
 * Efficient simplex indexing functions by Bill Licea-Kane, ATI. Thanks!
 */
void simplex( const in vec3 P, out vec3 offset1, out vec3 offset2 )
{
  vec3 offset0;

  vec2 isX = step( P.yz, P.xx );         // P.x >= P.y ? 1.0 : 0.0;  P.x >= P.z ? 1.0 : 0.0;
  offset0.x  = dot( isX, vec2( 1.0 ) );  // Accumulate all P.x >= other channels in offset.x
  offset0.yz = 1.0 - isX;                // Accumulate all P.x <  other channels in offset.yz

  float isY = step( P.z, P.y );          // P.y >= P.z ? 1.0 : 0.0;
  offset0.y += isY;                      // Accumulate P.y >= P.z in offset.y
  offset0.z += 1.0 - isY;                // Accumulate P.y <  P.z in offset.z

  // offset0 now contains the unique values 0,1,2 in each channel
  // 2 for the channel greater than other channels
  // 1 for the channel that is less than one but greater than another
  // 0 for the channel less than other channels
  // Equality ties are broken in favor of first x, then y
  // (z always loses ties)

  offset2 = clamp(   offset0, 0.0, 1.0 );
  // offset2 contains 1 in each channel that was 1 or 2
  offset1 = clamp( offset0-1.0, 0.0, 1.0 );
  // offset1 contains 1 in the single channel that was 1
}

/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */
float snoise3D1D(vec3 P)
{
// The skewing and unskewing factors are much simpler for the 3D case
#define F3 0.333333333333
#define G3 0.166666666667
// This is 1.0-3.0*G3, to remove a constant multiplication later
#define H3 0.5

  // Skew the (x,y,z) space to determine which cell of 6 simplices we're in
  float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
  vec3 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z) * G3;
  vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
  // To find out which of the six possible tetrahedra we're in, we need to
  // determine the magnitude ordering of x, y and z components of Pf0.
  vec3 o1;
  vec3 o2;
  simplex(Pf0, o1, o2);

  // Gradient at simplex origin
  float perm0 = texture2D(perlinTexture, Pi.xy).a;
  vec3  grad0 = texture2D(perlinTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
  // Gradient at second corner
  float perm1 = texture2D(perlinTexture, Pi.xy + o1.xy*ONE).a;
  vec3  grad1 = texture2D(perlinTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
  // Gradient at third corner
  float perm2 = texture2D(perlinTexture, Pi.xy + o2.xy*ONE).a;
  vec3  grad2 = texture2D(perlinTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
  // Gradient at last corner
  float perm3 = texture2D(perlinTexture, Pi.xy + vec2(ONE, ONE)).a;
  vec3  grad3 = texture2D(perlinTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;

  vec3 Pf1 = Pf0 - o1 + G3;
  vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
  vec3 Pf3 = Pf0 - vec3(H3);

  // Perform all four blend kernel computations in parallel on vec4 data
  vec4 n0123 = 0.6 - vec4(dot(Pf0, Pf0), dot(Pf1, Pf1), dot(Pf2, Pf2), dot(Pf3, Pf3));
  n0123 = max(n0123, 0.0);
  n0123 *= n0123;
  n0123 *= n0123 * vec4(dot(grad0, Pf0), dot(grad1, Pf1), dot(grad2, Pf2), dot(grad3, Pf3));

  // Sum up and scale the result to cover the range [-1,1]
  return 32.0 * (n0123.x + n0123.y + n0123.z + n0123.w);
}

/**
 * Returns 2 simplex noise values between -1 and +1, from a continuous 3D space.
 */
vec2 snoise3D2D(vec3 v)
{
	return vec2(snoise3D1D(vec3( v.x,  v.y,  v.z)),
				snoise3D1D(vec3(-v.x, -v.y, -v.z)));
}

/**
 * Returns 3 simplex noise values between -1 and +1, from a continuous 3D space.
 */
vec3 snoise3D3D(vec3 v)
{
	return vec3(snoise3D1D(vec3(v.x-1, v.y-1, v.z-1)),
				snoise3D1D(vec3(v.x,   v.y,   v.z)),
				snoise3D1D(vec3(v.x+1, v.y+1, v.z+1)));
}

/**
 * Returns 4 simplex noise values between -1 and +1, from a continuous 3D space.
 */
vec4 snoise3D4D(vec3 v)
{
	return vec4(snoise3D1D(vec3(v.x-1, v.y-1, v.z-1)),
				snoise3D1D(vec3(v.x,   v.y,   v.z)),
				snoise3D1D(vec3(v.x+1, v.y+1, v.z+1)),
				snoise3D1D(vec3(v.x+2, v.y+2, v.z+2)));
}

/**
 * Returns a multi-level (fractal) simplex noise value between -1 and +1, from a continuous 3D space.
 */
float snoise3D1DFractal(vec3 P, int levels, float roughness, float spacing)
{
	float intensity = 0.;
	float amplitude = 1.;
	float mixSum = 0.;
	for (int i = 0; i < levels; ++i)
	{
		vec3 nc = P;

		// Scale to the current octave.
		nc.xy *= pow(spacing, float(i));

		// Start each octave in a different position, so we don't see streaks when all octaves approach (0,0,0).
		nc.z += float(i);

		intensity += snoise3D1D(nc) * amplitude;

		mixSum += amplitude;
		amplitude *= roughness;
	}

	return intensity / mixSum;
}

/**
 * Returns 2 multi-level (fractal) simplex noise values between -1 and +1, from a continuous 3D space.
 */
vec2 snoise3D2DFractal(vec3 P, int levels, float roughness, float spacing)
{
	vec2 intensity = vec2(0.);
	float amplitude = 1.;
	float mixSum = 0.;
	for (int i = 0; i < levels; ++i)
	{
		vec3 nc = P;

		// Scale to the current octave.
		nc.xy *= pow(spacing, float(i));

		// Start each octave in a different position, so we don't see streaks when all octaves approach (0,0,0).
		nc.z += float(i);

		intensity += snoise3D2D(nc) * amplitude;

		mixSum += amplitude;
		amplitude *= roughness;
	}

	return intensity / mixSum;
}




//
// GLSL textureless classic 3D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-10-11
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise3D1D(vec3 P)
{
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}

// Classic Perlin noise, periodic variant
float pnoise3D1D(vec3 P, vec3 rep)
{
  vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
  vec3 Pi1 = mod(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}

/**
 * Returns a multi-level (fractal) classic-Perlin noise value between -1 and +1, from a continuous 3D space.
 */
float cnoise3D1DFractal(vec3 P, int levels, float roughness, float spacing)
{
	float intensity = 0.;
	float amplitude = 1.;
	float mixSum = 0.;
	for (int i = 0; i < levels; ++i)
	{
		vec3 nc = P;

		// Scale to the current octave.
		nc.xy *= pow(spacing, float(i));

		// Start each octave in a different position, so we don't see streaks when all octaves approach (0,0,0).
		nc.z += float(i);

		intensity += cnoise3D1D(nc) * amplitude;

		mixSum += amplitude;
		amplitude *= roughness;
	}

	return intensity / mixSum;
}




// Adapted from http://catlikecoding.com/unity/tutorials/simplex-noise/
// by Jasper Flick
// "Can I use your tutorial code for commercial projects?
// Go ahead! You're free to build on the tutorial code however you want,
// using it for any purpose. You don't need to credit me, but that sure is appreciated!"
int hash(int h)
{
	return int(texture2D(perlinTexture, vec2(float(mod(h, 255))/256.,0.)).a * 255.);
}

float SimplexValue3DPart(vec3 point, int ix, int iy, int iz) {
	float unskew = (ix + iy + iz) * (1. / 6.);
	float x = point.x - ix + unskew;
	float y = point.y - iy + unskew;
	float z = point.z - iz + unskew;
	float f = 0.5 - x * x - y * y - z * z;
	float sample = 0.;
	if (f > 0.) {
		float f2 = f * f;
		float f3 = f * f2;
		float h = hash(hash(hash(ix) + iy) + iz);
		float h6f2 = -6. * h * f2;
		sample = h * f3;
	}
	return sample;
}

// Returns a sample (-1 to 1) from 3D Value Noise space.
float SimplexValue3D(vec3 point) {
	float skew = (point.x + point.y + point.z) * (1. / 3.);
	float sx = point.x + skew;
	float sy = point.y + skew;
	float sz = point.z + skew;
	int ix = int(floor(sx));
	int iy = int(floor(sy));
	int iz = int(floor(sz));
	float sample = SimplexValue3DPart(point, ix, iy, iz);
	sample += SimplexValue3DPart(point, ix + 1, iy + 1, iz + 1);
	float x = sx - ix;
	float y = sy - iy;
	float z = sz - iz;
	if (x >= y) {
		if (x >= z) {
			sample += SimplexValue3DPart(point, ix + 1, iy, iz);
			if (y >= z) {
				sample += SimplexValue3DPart(point, ix + 1, iy + 1, iz);
			}
			else {
				sample += SimplexValue3DPart(point, ix + 1, iy, iz + 1);
			}
		}
		else {
			sample += SimplexValue3DPart(point, ix, iy, iz + 1);
			sample += SimplexValue3DPart(point, ix + 1, iy, iz + 1);
		}
	}
	else {
		if (y >= z) {
			sample += SimplexValue3DPart(point, ix, iy + 1, iz);
			if (x >= z) {
				sample += SimplexValue3DPart(point, ix + 1, iy + 1, iz);
			}
			else {
				sample += SimplexValue3DPart(point, ix, iy + 1, iz + 1);
			}
		}
		else {
			sample += SimplexValue3DPart(point, ix, iy, iz + 1);
			sample += SimplexValue3DPart(point, ix, iy + 1, iz + 1);
		}
	}
	return sample * (8. * 2. / 255.) - 1.;
}
