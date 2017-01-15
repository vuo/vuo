/**
 * @file
 * VuoGradientNoiseCommon implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoGradientNoiseCommon.h"
#include <math.h>


#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoGradientNoiseCommon",
					  "dependencies" : [
						"VuoReal",
						"VuoPoint2d",
						"VuoPoint3d",
						"VuoPoint4d"
					  ]
				  });
#endif


/**
 * Perlin's shuffled ordering of 0..255, repeated once.
 */
static unsigned char perm[] =
{
	151, 160, 137, 91,
				90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103,
				30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120,
				234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57,
				177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68,
				175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83,
				111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245,
				40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76,
				132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
				164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
				5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47,
				16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2,
				44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39,
				253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218,
				246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162,
				241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181,
				199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
				254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128,
				195, 78, 66, 215, 61, 156, 180,
	151, 160, 137, 91,
				90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103,
				30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120,
				234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57,
				177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68,
				175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83,
				111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245,
				40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76,
				132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86,
				164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
				5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47,
				16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2,
				44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39,
				253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218,
				246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162,
				241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181,
				199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150,
				254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128,
				195, 78, 66, 215, 61, 156, 180
};

/**
 * Lookup table for simplex traversal.
 */
static unsigned char simplex[64][4] =
{
	{0,1,2,3},{0,1,3,2},{0,0,0,0},{0,2,3,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,2,3,0},
	{0,2,1,3},{0,0,0,0},{0,3,1,2},{0,3,2,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,3,2,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{1,2,0,3},{0,0,0,0},{1,3,0,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,3,0,1},{2,3,1,0},
	{1,0,2,3},{1,0,3,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,0,3,1},{0,0,0,0},{2,1,3,0},
	{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
	{2,0,1,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,0,1,2},{3,0,2,1},{0,0,0,0},{3,1,2,0},
	{2,1,0,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,1,0,2},{0,0,0,0},{3,2,0,1},{3,2,1,0}
};


static inline VuoReal grad1d(int hash, VuoReal x);
static inline VuoReal grad2dPerlin(int hash, VuoReal x, VuoReal y);
static inline VuoReal grad2dSimplex(int hash, VuoReal x, VuoReal y);
static inline VuoReal grad3d(int hash, VuoReal x, VuoReal y, VuoReal z);
static inline VuoReal grad4dPerlin(int hash, VuoReal x, VuoReal y, VuoReal z, VuoReal w);
static inline VuoReal grad4dSimplex(int hash, VuoReal x, VuoReal y, VuoReal z, VuoReal t);

static inline VuoReal fade(VuoReal t);
static inline VuoReal lerp(VuoReal t, VuoReal a, VuoReal b);


/**
 * Returns the value for Perlin noise at a 1D location.
 */
VuoReal VuoGradientNoise_perlin_VuoReal_VuoReal(VuoReal x)
{
	VuoInteger X;
	VuoReal u;
	VuoInteger A, B;

	X = (int)floor(x) & 255;	// Integer part of x
	x = x - floor(x);			// Fractional part of x
	u = fade(x);

	A = perm[X  ];
	B = perm[X+1];

	return 0.25 * lerp(u,
				grad1d(perm[A], x),
				grad1d(perm[B], x-1));
}

/**
 * Returns 2 values for Perlin noise at a 1D location.
 */
VuoPoint2d VuoGradientNoise_perlin_VuoReal_VuoPoint2d(VuoReal x)
{
	return VuoPoint2d_make(VuoGradientNoise_perlin_VuoReal_VuoReal(x),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x+1));
}

/**
 * Returns 3 values for Perlin noise at a 1D location.
 */
VuoPoint3d VuoGradientNoise_perlin_VuoReal_VuoPoint3d(VuoReal x)
{
	return VuoPoint3d_make(VuoGradientNoise_perlin_VuoReal_VuoReal(x-1),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x+1));
}

/**
 * Returns 4 values for Perlin noise at a 1D location.
 */
VuoPoint4d VuoGradientNoise_perlin_VuoReal_VuoPoint4d(VuoReal x)
{
	return VuoPoint4d_make(VuoGradientNoise_perlin_VuoReal_VuoReal(x-1),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x+1),
						   VuoGradientNoise_perlin_VuoReal_VuoReal(x+2));
}

/**
 * Returns the value for Perlin noise at a 2D location.
 */
VuoReal VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;

	VuoInteger X, Y;
	VuoReal u, v;
	VuoInteger A, AA, AB, B, BA, BB;

	X = (int)floor(x) & 255;
	Y = (int)floor(y) & 255;

	x = x - floor(x);
	y = y - floor(y);

	u = fade(x);
	v = fade(y);

	A  = perm[X  ] + Y;
	AA = perm[A  ]    ;
	AB = perm[A+1]    ;
	B  = perm[X+1] + Y;
	BA = perm[B  ]    ;
	BB = perm[B+1]    ;

	return lerp(v,
				lerp(u,
					 grad2dPerlin(perm[AA], x  , y  ),
					 grad2dPerlin(perm[BA], x-1, y  )),
				lerp(u,
					 grad2dPerlin(perm[AB], x  , y-1),
					 grad2dPerlin(perm[BB], x-1, y-1)));
}

/**
 * Returns 2 values for Perlin noise at a 2D location.
 */
VuoPoint2d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint2d(VuoPoint2d point)
{
	return VuoPoint2d_make(VuoGradientNoise_perlin_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_multiply(point,-1)));
}

/**
 * Returns 3 values for Perlin noise at a 2D location.
 */
VuoPoint3d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint3d(VuoPoint2d point)
{
	return VuoPoint3d_make(VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(1,1))));
}

/**
 * Returns 4 values for Perlin noise at a 2D location.
 */
VuoPoint4d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint4d(VuoPoint2d point)
{
	return VuoPoint4d_make(VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(1,1))),
						   VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(2,2))));
}

/**
 * Returns the value for Perlin noise at a 3D location.
 */
VuoReal VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;
	VuoReal z = point.z;

	VuoInteger X, Y, Z;
	VuoReal u, v, w;
	VuoInteger A, AA, AB, B, BA, BB;

	X = (int)floor(x) & 255;	// Integer part of x
	Y = (int)floor(y) & 255;	// Integer part of y
	Z = (int)floor(z) & 255;	// Integer part of z

	x = x - floor(x);			// Fractional part of x
	y = y - floor(y);			// Fractional part of y
	z = z - floor(z);			// Fractional part of z
	u = fade(x);
	v = fade(y);
	w = fade(z);

	A  = perm[X  ] + Y;
	AA = perm[A  ] + Z;
	AB = perm[A+1] + Z;
	B  = perm[X+1] + Y;
	BA = perm[B  ] + Z;
	BB = perm[B+1] + Z;

	return lerp(w,
				lerp(v,
					 lerp(u,
						  grad3d(perm[AA  ], x  , y  , z),
						  grad3d(perm[BA  ], x-1, y  , z)),
					 lerp(u,
						  grad3d(perm[AB  ], x  , y-1, z),
						  grad3d(perm[BB  ], x-1, y-1, z))),
				lerp(v,
					 lerp(u,
						  grad3d(perm[AA+1], x  , y  , z-1),
						  grad3d(perm[BA+1], x-1, y  , z-1)),
					 lerp(u,
						  grad3d(perm[AB+1], x  , y-1, z-1),
						  grad3d(perm[BB+1], x-1, y-1, z-1))));
}

/**
 * Returns 2 values for Perlin noise at a 3D location.
 */
VuoPoint2d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint2d(VuoPoint3d point)
{
	return VuoPoint2d_make(VuoGradientNoise_perlin_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_multiply(point,-1)));
}

/**
 * Returns 3 values for Perlin noise at a 3D location.
 */
VuoPoint3d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint3d(VuoPoint3d point)
{
	return VuoPoint3d_make(VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(-1,-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(1,1,1))));
}

/**
 * Returns 4 values for Perlin noise at a 3D location.
 */
VuoPoint4d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint4d(VuoPoint3d point)
{
	return VuoPoint4d_make(VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(-1,-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(1,1,1))),
						   VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(2,2,2))));
}

/**
 * Returns the value for Perlin noise at a 4D location.
 */
VuoReal VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;
	VuoReal z = point.z;
	VuoReal w = point.w;

	VuoInteger X, Y, Z, W;
	VuoReal a, b, c, d;
	VuoInteger A, B,
			AA, AB, BA, BB,
			AAA, AAB, ABA, ABB,
			BAA, BAB, BBA, BBB;

	X = (int)floor(x) & 255;
	Y = (int)floor(y) & 255;
	Z = (int)floor(z) & 255;
	W = (int)floor(w) & 255;

	x = x - floor(x);
	y = y - floor(y);
	z = z - floor(z);
	w = w - floor(w);

	a = fade(x);
	b = fade(y);
	c = fade(z);
	d = fade(w);

	A   = perm[X   ]+Y;
	AA  = perm[A   ]+Z;
	AB  = perm[A +1]+Z;
	B   = perm[X +1]+Y;
	BA  = perm[B   ]+Z;
	BB  = perm[B +1]+Z;
	AAA = perm[AA  ]+W;
	AAB = perm[AA+1]+W;
	ABA = perm[AB  ]+W;
	ABB = perm[AB+1]+W;
	BAA = perm[BA  ]+W;
	BAB = perm[BA+1]+W;
	BBA = perm[BB  ]+W;
	BBB = perm[BB+1]+W;

	return lerp(d,
				lerp(c,
					 lerp(b,
						  lerp(a,
							   grad4dPerlin(perm[AAA  ], x  , y  , z  , w  ),
							   grad4dPerlin(perm[BAA  ], x-1, y  , z  , w  )),
						  lerp(b,
							   grad4dPerlin(perm[ABA  ], x  , y  , z  , w  ),
							   grad4dPerlin(perm[BBA  ], x-1, y-1, z  , w  ))),
					 lerp(b,
						  lerp(a,
							   grad4dPerlin(perm[AAB  ], x  , y  , z-1, w  ),
							   grad4dPerlin(perm[BAB  ], x-1, y  , z-1, w  )),
						  lerp(a,
							   grad4dPerlin(perm[ABB  ], x  , y-1, z-1, w  ),
							   grad4dPerlin(perm[BBB  ], x-1, y-1, z-1, w  )))),
				lerp(c,
					 lerp(b,
						  lerp(a,
							   grad4dPerlin(perm[AAA+1], x  , y  , z  , w-1),
							   grad4dPerlin(perm[BAA+1], x-1, y  , z  , w-1)),
						  lerp(b,
							   grad4dPerlin(perm[ABA+1], x  , y  , z  , w-1),
							   grad4dPerlin(perm[BBA+1], x-1, y-1, z  , w-1))),
					 lerp(b,
						  lerp(a,
							   grad4dPerlin(perm[AAB+1], x  , y  , z-1, w-1),
							   grad4dPerlin(perm[BAB+1], x-1, y  , z-1, w-1)),
						  lerp(a,
							   grad4dPerlin(perm[ABB+1], x  , y-1, z-1, w-1),
							   grad4dPerlin(perm[BBB+1], x-1, y-1, z-1, w-1)))));
}

/**
 * Returns 2 values for Perlin noise at a 4D location.
 */
VuoPoint2d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint2d(VuoPoint4d point)
{
	return VuoPoint2d_make(VuoGradientNoise_perlin_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_multiply(point,-1)));
}

/**
 * Returns 3 values for Perlin noise at a 4D location.
 */
VuoPoint3d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint3d(VuoPoint4d point)
{
	return VuoPoint3d_make(VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(-1,-1,-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(1,1,1,1))));
}

/**
 * Returns 4 values for Perlin noise at a 4D location.
 */
VuoPoint4d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint4d(VuoPoint4d point)
{
	return VuoPoint4d_make(VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(-1,-1,-1,-1))),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(1,1,1,1))),
						   VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(2,2,2,2))));
}

/**
 * Returns the value for Simplex noise at a 1D location.
 */
VuoReal VuoGradientNoise_simplex_VuoReal_VuoReal(VuoReal x)
{
	VuoInteger i0 = (int)floor(x);
	VuoInteger i1 = i0 + 1;
	VuoReal x0 = x - i0;
	VuoReal x1 = x0 - 1.0f;

	VuoReal n0, n1;

	float t0 = 1.0f - x0 * x0;
	t0 *= t0;
	n0 = t0 * t0 * grad1d(perm[i0 & 0xff], x0);

	float t1 = 1.0f - x1 * x1;
	t1 *= t1;
	n1 = t1 * t1 * grad1d(perm[i1 & 0xff], x1);
	return 0.395f * (n0 + n1);
}

/**
 * Returns 2 values for Simplex noise at a 1D location.
 */
VuoPoint2d VuoGradientNoise_simplex_VuoReal_VuoPoint2d(VuoReal x)
{
	return VuoPoint2d_make(VuoGradientNoise_simplex_VuoReal_VuoReal(x),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x+1));
}

/**
 * Returns 3 values for Simplex noise at a 1D location.
 */
VuoPoint3d VuoGradientNoise_simplex_VuoReal_VuoPoint3d(VuoReal x)
{
	return VuoPoint3d_make(VuoGradientNoise_simplex_VuoReal_VuoReal(x-1),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x+1));
}

/**
 * Returns 4 values for Simplex noise at a 1D location.
 */
VuoPoint4d VuoGradientNoise_simplex_VuoReal_VuoPoint4d(VuoReal x)
{
	return VuoPoint4d_make(VuoGradientNoise_simplex_VuoReal_VuoReal(x-1),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x+1),
						   VuoGradientNoise_simplex_VuoReal_VuoReal(x+2));
}

/**
 * Returns the value for Simplex noise at a 2D location.
 */
VuoReal VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;

	VuoReal f2 = 0.366025403; // 0.5 * (sqrt(3.0) - 1.0)
	VuoReal g2 = 0.211324865; // (3.0 - sqrt(3.0)) / 6.0

	// Noise contributions from three corners
	VuoReal n0, n1, n2;

	// Skew the input space to determine which simplex cell we're in
	VuoReal s = (x + y) * f2; // Hairy factor for 2D
	VuoReal xs = x + s;
	VuoReal ys = y + s;

	VuoInteger i = (int)floor(xs);
	VuoInteger j = (int)floor(ys);

	VuoReal t = (VuoReal) (i + j) * g2;
	// Unskew the cell origin back to (x, y) space
	VuoReal X0 = i - t;
	VuoReal Y0 = j - t;
	// The x, y distances from the cell origin
	VuoReal x0 = x - X0;
	VuoReal y0 = y - Y0;

	// For the 2D case, the simplex shape is an equilateral triangle.
	// Determine which simplex we are in.

	// Offsets for second (middle) corner of simplex in (i, j) coords
	VuoInteger i1, j1;
	// Lower triangle, XY order: (0,0)->(1,0)->(1,1)
	if (x0 > y0)
	{
		i1 = 1;
		j1 = 0;
	}
	// Upper triangle, YX order: (0,0)->(0,1)->(1,1)
	else
	{
		i1 = 0;
		j1 = 1;
	}

	// Offsets for middle corner in (x, y) unskewed coords
	VuoReal x1 = x0 - i1 + g2;
	VuoReal y1 = y0 - j1 + g2;
	// Ofsets for last corner in (x, y) unskewed coords
	VuoReal x2 = x0 - 1.0f + 2.0f * g2;
	VuoReal y2 = y0 - 1.0f + 2.0f * g2;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	VuoInteger ii = i & 0xff;
	VuoInteger jj = j  & 0xff;

	// Calculate the contribution from the three corners
	VuoReal t0 = 0.5f - x0 * x0 - y0 * y0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad2dSimplex(perm[ii + perm[jj]], x0, y0);
	}

	VuoReal t1 = 0.5f - x1 * x1 - y1 * y1;
	if (t1 < 0.0f)
		n1 = 0.0f;
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad2dSimplex(perm[ii + i1 + perm[jj + j1]], x1, y1);
	}

	VuoReal t2 = 0.5f - x2 * x2 - y2 * y2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad2dSimplex(perm[ii + 1 + perm[jj + 1]], x2, y2);
	}

	// Add contributions from each corner to the final noise value.
	// The result should be scaled to return values in the interval [-1, 1].
	return 46.0f * (n0 + n1 + n2);
}

/**
 * Returns 2 values for Simplex noise at a 2D location.
 */
VuoPoint2d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint2d(VuoPoint2d point)
{
	return VuoPoint2d_make(VuoGradientNoise_simplex_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_multiply(point,-1)));
}

/**
 * Returns 3 values for Simplex noise at a 2D location.
 */
VuoPoint3d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint3d(VuoPoint2d point)
{
	return VuoPoint3d_make(VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(1,1))));
}

/**
 * Returns 4 values for Simplex noise at a 2D location.
 */
VuoPoint4d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint4d(VuoPoint2d point)
{
	return VuoPoint4d_make(VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(1,1))),
						   VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d_add(point,VuoPoint2d_make(2,2))));
}

/**
 * Returns the value for Simplex noise at a 3D location.
 */
VuoReal VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;
	VuoReal z = point.z;

	// Simple skewing factors for the 3D case
	VuoReal F3 = 0.333333333;
	VuoReal G3 = 0.166666667;

	VuoReal n0, n1, n2, n3; // Noise contributions from the four corners

	// Skew the input space to determine which simplex cell we're in
	VuoReal s = (x + y + z) * F3; // Very nice and simple skew factor for 3D
	VuoReal xs = x + s;
	VuoReal ys = y + s;
	VuoReal zs = z + s;
	VuoInteger i = (int)floor(xs);
	VuoInteger j = (int)floor(ys);
	VuoInteger k = (int)floor(zs);

	VuoReal t = (VuoReal)(i + j + k) * G3;
	VuoReal X0 = i - t; // Unskew the cell origin back to (x,y,z) space
	VuoReal Y0 = j - t;
	VuoReal Z0 = k - t;
	VuoReal x0 = x - X0; // The x,y,z distances from the cell origin
	VuoReal y0 = y - Y0;
	VuoReal z0 = z - Z0;

	// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	// Determine which simplex we are in.
	VuoInteger i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
	VuoInteger i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

	if (x0 >= y0)
	{
		if (y0 >= z0)
		{
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} // X Y Z order
		else if (x0 >= z0)
		{
			i1 = 1;
			j1 = 0;
			k1 = 0;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} // X Z Y order
		else
		{
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 1;
			j2 = 0;
			k2 = 1;
		} // Z X Y order
	}
	else
	{ // x0 < y0
		if (y0 < z0)
		{
			i1 = 0;
			j1 = 0;
			k1 = 1;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} // Z Y X order
		else if (x0 < z0)
		{
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 0;
			j2 = 1;
			k2 = 1;
		} // Y Z X order
		else {
			i1 = 0;
			j1 = 1;
			k1 = 0;
			i2 = 1;
			j2 = 1;
			k2 = 0;
		} // Y X Z order
	}

	// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	// c = 1/6.

	VuoReal x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
	VuoReal y1 = y0 - j1 + G3;
	VuoReal z1 = z0 - k1 + G3;
	VuoReal x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
	VuoReal y2 = y0 - j2 + 2.0f * G3;
	VuoReal z2 = z0 - k2 + 2.0f * G3;
	VuoReal x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
	VuoReal y3 = y0 - 1.0f + 3.0f * G3;
	VuoReal z3 = z0 - 1.0f + 3.0f * G3;

	// Wrap the VuoIntegereger indices at 256, to avoid indexing perm[] out of bounds
	VuoInteger ii = i & 0xff;
	VuoInteger jj = j & 0xff;
	VuoInteger kk = k & 0xff;

	// Calculate the contribution from the four corners
	VuoReal t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad3d(perm[ii+perm[jj+perm[kk]]], x0, y0, z0);
	}

	VuoReal t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
	if (t1 < 0.0f)
		n1 = 0.0f;
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad3d(perm[ii+i1+perm[jj+j1+perm[kk+k1]]], x1, y1, z1);
	}

	VuoReal t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad3d(perm[ii+i2+perm[jj+j2+perm[kk+k2]]], x2, y2, z2);
	}

	VuoReal t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
	if (t3 < 0.0f)
		n3 = 0.0f;
	else
	{
		t3 *= t3;
		n3 = t3 * t3 * grad3d(perm[ii+1+perm[jj+1+perm[kk+1]]], x3, y3, z3);
	}

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to stay just inside [-1,1]
	return 32.0f * (n0 + n1 + n2 + n3); // TODO: The scale factor is preliminary!
}

/**
 * Returns 2 values for Simplex noise at a 3D location.
 */
VuoPoint2d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint2d(VuoPoint3d point)
{
	return VuoPoint2d_make(VuoGradientNoise_simplex_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_multiply(point,-1)));
}

/**
 * Returns 3 values for Simplex noise at a 3D location.
 */
VuoPoint3d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint3d(VuoPoint3d point)
{
	return VuoPoint3d_make(VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(-1,-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(1,1,1))));
}

/**
 * Returns 4 values for Simplex noise at a 3D location.
 */
VuoPoint4d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint4d(VuoPoint3d point)
{
	return VuoPoint4d_make(VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(-1,-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(1,1,1))),
						   VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d_add(point,VuoPoint3d_make(2,2,2))));
}

/**
 * Returns the value for Simplex noise at a 4D location.
 */
VuoReal VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d point)
{
	VuoReal x = point.x;
	VuoReal y = point.y;
	VuoReal z = point.z;
	VuoReal w = point.w;

	// The skewing and unskewing factors are hairy again for the 4D case
	VuoReal F4 = 0.309016994; // F4 = (Math.sqrt(5.0)-1.0)/4.0
	VuoReal G4 = 0.138196601; // G4 = (5.0-Math.sqrt(5.0))/20.0

	VuoReal n0, n1, n2, n3, n4; // Noise contributions from the five corners

	// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
	VuoReal s = (x + y + z + w) * F4; // Factor for 4D skewing
	VuoReal xs = x + s;
	VuoReal ys = y + s;
	VuoReal zs = z + s;
	VuoReal ws = w + s;
	VuoInteger i = (int)floor(xs);
	VuoInteger j = (int)floor(ys);
	VuoInteger k = (int)floor(zs);
	VuoInteger l = (int)floor(ws);

	VuoReal t = (i + j + k + l) * G4; // Factor for 4D unskewing
	VuoReal X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
	VuoReal Y0 = j - t;
	VuoReal Z0 = k - t;
	VuoReal W0 = l - t;

	VuoReal x0 = x - X0;  // The x,y,z,w distances from the cell origin
	VuoReal y0 = y - Y0;
	VuoReal z0 = z - Z0;
	VuoReal w0 = w - W0;

	  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
	  // To find out which of the 24 possible simplices we're in, we need to
	  // determine the magnitude ordering of x0, y0, z0 and w0.
	  // The method below is a good way of finding the ordering of x,y,z,w and
	  // then find the correct traversal order for the simplex we’re in.
	  // First, six pair-wise comparisons are performed between each possible pair
	  // of the four coordinates, and the results are used to add up binary bits
	  // for an integer index.
	VuoInteger c1 = (x0 > y0) ? 32 : 0;
	VuoInteger c2 = (x0 > z0) ? 16 : 0;
	VuoInteger c3 = (y0 > z0) ? 8 : 0;
	VuoInteger c4 = (x0 > w0) ? 4 : 0;
	VuoInteger c5 = (y0 > w0) ? 2 : 0;
	VuoInteger c6 = (z0 > w0) ? 1 : 0;
	VuoInteger c = c1 + c2 + c3 + c4 + c5 + c6;

	VuoInteger i1, j1, k1, l1; // The integer offsets for the second simplex corner
	VuoInteger i2, j2, k2, l2; // The integer offsets for the third simplex corner
	VuoInteger i3, j3, k3, l3; // The integer offsets for the fourth simplex corner

	// simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
	// Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
	// impossible. Only the 24 indices which have non-zero entries make any sense.
	// We use a thresholding to set the coordinates in turn from the largest magnitude.
	// The number 3 in the "simplex" array is at the position of the largest coordinate.
	i1 = simplex[c][0] >= 3 ? 1 : 0;
	j1 = simplex[c][1] >= 3 ? 1 : 0;
	k1 = simplex[c][2] >= 3 ? 1 : 0;
	l1 = simplex[c][3] >= 3 ? 1 : 0;
	// The number 2 in the "simplex" array is at the second largest coordinate.
	i2 = simplex[c][0] >= 2 ? 1 : 0;
	j2 = simplex[c][1] >= 2 ? 1 : 0;
	k2 = simplex[c][2] >= 2 ? 1 : 0;
	l2 = simplex[c][3] >= 2 ? 1 : 0;
	// The number 1 in the "simplex" array is at the second smallest coordinate.
	i3 = simplex[c][0]>=1 ? 1 : 0;
	j3 = simplex[c][1]>=1 ? 1 : 0;
	k3 = simplex[c][2]>=1 ? 1 : 0;
	l3 = simplex[c][3]>=1 ? 1 : 0;
	// The fifth corner has all coordinate offsets = 1, so no need to look that up.

	VuoReal x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
	VuoReal y1 = y0 - j1 + G4;
	VuoReal z1 = z0 - k1 + G4;
	VuoReal w1 = w0 - l1 + G4;
	VuoReal x2 = x0 - i2 + 2.0f*G4; // Offsets for third corner in (x,y,z,w) coords
	VuoReal y2 = y0 - j2 + 2.0f*G4;
	VuoReal z2 = z0 - k2 + 2.0f*G4;
	VuoReal w2 = w0 - l2 + 2.0f*G4;
	VuoReal x3 = x0 - i3 + 3.0f*G4; // Offsets for fourth corner in (x,y,z,w) coords
	VuoReal y3 = y0 - j3 + 3.0f*G4;
	VuoReal z3 = z0 - k3 + 3.0f*G4;
	VuoReal w3 = w0 - l3 + 3.0f*G4;
	VuoReal x4 = x0 - 1.0f + 4.0f*G4; // Offsets for last corner in (x,y,z,w) coords
	VuoReal y4 = y0 - 1.0f + 4.0f*G4;
	VuoReal z4 = z0 - 1.0f + 4.0f*G4;
	VuoReal w4 = w0 - 1.0f + 4.0f*G4;

	// Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
	VuoInteger ii = i & 0xff;
	VuoInteger jj = j & 0xff;
	VuoInteger kk = k & 0xff;
	VuoInteger ll = l & 0xff;

	// Calculate the contribution from the five corners
	VuoReal t0 = 0.6f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
	if (t0 < 0.0f)
		n0 = 0.0f;
	else
	{
		t0 *= t0;
		n0 = t0 * t0 * grad4dSimplex(perm[ii + perm[jj + perm[kk + perm[ll]]]], x0, y0, z0, w0);
	}

	VuoReal t1 = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
	if(t1 < 0.0f)
		n1 = 0.0f;
	else
	{
		t1 *= t1;
		n1 = t1 * t1 * grad4dSimplex(perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]], x1, y1, z1, w1);
	}

	VuoReal t2 = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
	if (t2 < 0.0f)
		n2 = 0.0f;
	else
	{
		t2 *= t2;
		n2 = t2 * t2 * grad4dSimplex(perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]], x2, y2, z2, w2);
	}

	VuoReal t3 = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
	if (t3 < 0.0f)
		n3 = 0.0f;
	else
	{
		t3 *= t3;
		n3 = t3 * t3 * grad4dSimplex(perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]], x3, y3, z3, w3);
	}

	VuoReal t4 = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
	if (t4 < 0.0f)
		n4 = 0.0f;
	else
	{
		t4 *= t4;
		n4 = t4 * t4 * grad4dSimplex(perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]], x4, y4, z4, w4);
	}

	// Sum up and scale the result to cover the range [-1,1]
	return 27.0f * (n0 + n1 + n2 + n3 + n4); // TODO: The scale factor is preliminary!
}

/**
 * Returns 2 values for Simplex noise at a 4D location.
 */
VuoPoint2d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint2d(VuoPoint4d point)
{
	return VuoPoint2d_make(VuoGradientNoise_simplex_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_multiply(point,-1)));
}

/**
 * Returns 3 values for Simplex noise at a 4D location.
 */
VuoPoint3d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint3d(VuoPoint4d point)
{
	return VuoPoint3d_make(VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(-1,-1,-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(1,1,1,1))));
}

/**
 * Returns 4 values for Simplex noise at a 4D location.
 */
VuoPoint4d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint4d(VuoPoint4d point)
{
	return VuoPoint4d_make(VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(-1,-1,-1,-1))),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(point),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(1,1,1,1))),
						   VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d_add(point,VuoPoint4d_make(2,2,2,2))));
}

/**
 * Returns pseudo-random gradient value for given 1d point.
 */
static inline VuoReal grad1d(int hash, VuoReal x)
{
	int h = hash & 15;

	/*
	 * Gradient value 1.0, 2.0, ..., 8.0
	 * and a random sign for the gradient
	 */
	float grad = 1.0 + (h & 7);
	if (h & 8)
	{
		grad = -grad;
	}

	// Multiply the gradient with the distance
	return grad * x;
}

/**
 * Returns pseudo-random gradient value for given 2d point for the Perlin algorithm.
 */
static inline VuoReal grad2dPerlin(int hash, VuoReal x, VuoReal y)
{
	VuoInteger h = hash & 15;
	VuoReal u = h < 8 ? x : y;
	VuoReal v = h < 4 ? y : h==12 || h==14 ? x : 0;
	return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

/**
 * Returns a pseudo-random gradient value for given 2d point for the Simplex algorithm.
 */
static inline VuoReal grad2dSimplex(int hash, VuoReal x, VuoReal y)
{
	VuoInteger h = hash & 7;
	VuoReal u = h < 4 ? x : y;
	VuoReal v = h < 4 ? y : x;
	return ((h&1) ? -u : u) + ((h&2) ? -2.0f*v : 2.0f*v);
}

/**
 * Returns pseudo-random gradient value for given 3d point.
 */
static inline VuoReal grad3d(int hash, VuoReal x, VuoReal y, VuoReal z)
{
	VuoInteger h = hash & 15;
	VuoReal u = h<8 ? x : y;
	VuoReal v = h<4 ? y : h==12||h==14 ? x : z;
	return ((h&1) == 0 ? u : - u) + ((h&2) == 0 ? v : -v);
}

/**
 * Returns pseudo-random gradient value for given 4d point for Perlin noise.
 */
static inline VuoReal grad4dPerlin(int hash, VuoReal x, VuoReal y, VuoReal z, VuoReal w)
{
	VuoInteger h = hash & 31; // CONVERT LO 5 BITS OF HASH TO 32 GRAD DIRECTIONS.
	VuoReal a=y,b=z,c=w;            // X,Y,Z
	switch (h >> 3) {          // OR, DEPENDING ON HIGH ORDER 2 BITS:
		case 1:
			a=w;
			b=x;
			c=y;
			break;     // W,X,Y
		case 2:
			a=z;
			b=w;
			c=x;
			break;     // Z,W,X
		case 3:
			a=y;
			b=z;
			c=w;
			break;     // Y,Z,W
	}

	return ((h&4)==0 ? -a:a) + ((h&2)==0 ? -b:b) + ((h&1)==0 ? -c:c);
}

/**
 * Returns pseudo-random gradient value for given 4d point for Simplex noise.
 */
static inline VuoReal grad4dSimplex(int hash, VuoReal x, VuoReal y, VuoReal z, VuoReal t)
{
	VuoInteger h = hash & 31;
	VuoReal u = h < 24 ? x : y;
	VuoReal v = h < 16 ? y : z;
	VuoReal w = h < 8 ? z : t;
	return ((h&1) ? -u : u) + ((h&2) ? -v : v) + ((h&4) ? -w : w);
}

/**
 * Returns the value of the fade function of t.
 */
static inline VuoReal fade(VuoReal t)
{
	return (t * t * t * (t * (t * 6 - 15) + 10));
}

/**
 * Performs and returns linear interpolation on t, a, and b.
 */
static inline VuoReal lerp(VuoReal t, VuoReal a, VuoReal b)
{
	return (a + t * (b - a));
}


