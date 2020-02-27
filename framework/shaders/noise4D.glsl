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
uniform sampler2D gradTexture;

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
void simplex( const in vec4 P, out vec4 offset1, out vec4 offset2, out vec4 offset3 )
{
  vec4 offset0;

  vec3 isX = step( P.yzw, P.xxx );        // See comments in 3D simplex function
  offset0.x = dot( isX, vec3( 1.0 ) );
  offset0.yzw = 1.0 - isX;

  vec3 isYZ = step( P.zww, P.yyz );
  offset0.y += dot( isYZ.xy, vec2( 1.0 ) );
  offset0.zw += 1.0 - isYZ.xy;

  offset0.z += isYZ.z;
  offset0.w += 1.0 - isYZ.z;

  // offset0 now contains the unique values 0,1,2,3 in each channel

  offset3 = clamp( offset0,     0.0, 1.0 );
  offset2 = clamp( offset0-1.0, 0.0, 1.0 );
  offset1 = clamp( offset0-2.0, 0.0, 1.0 );
}


/**
 * 4D simplex noise. A lot faster than classic 4D noise, and better looking.
 */
float snoise4D1D(vec4 P) {

// The skewing and unskewing factors are hairy again for the 4D case
// This is (sqrt(5.0)-1.0)/4.0
#define F4 0.309016994375
// This is (5.0-sqrt(5.0))/20.0
#define G4 0.138196601125

  // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
	float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
  vec4 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
  vec4 P0 = Pi - t; // Unskew the cell origin back to (x,y,z,w) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec4 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
  // To find out which of the 24 possible simplices we're in, we need to
  // determine the magnitude ordering of x, y, z and w components of Pf0.
  vec4 o1;
  vec4 o2;
  vec4 o3;
  simplex(Pf0, o1, o2, o3);

  // Noise contribution from simplex origin
  float perm0xy = texture2D(perlinTexture, Pi.xy).a;
  float perm0zw = texture2D(perlinTexture, Pi.zw).a;
  vec4  grad0 = texture2D(gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
	t0 *= t0;
	n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec4 Pf1 = Pf0 - o1 + G4;
  o1 = o1 * ONE;
  float perm1xy = texture2D(perlinTexture, Pi.xy + o1.xy).a;
  float perm1zw = texture2D(perlinTexture, Pi.zw + o1.zw).a;
  vec4  grad1 = texture2D(gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
	t1 *= t1;
	n1 = t1 * t1 * dot(grad1, Pf1);
  }

  // Noise contribution from third corner
  vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
  o2 = o2 * ONE;
  float perm2xy = texture2D(perlinTexture, Pi.xy + o2.xy).a;
  float perm2zw = texture2D(perlinTexture, Pi.zw + o2.zw).a;
  vec4  grad2 = texture2D(gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
	t2 *= t2;
	n2 = t2 * t2 * dot(grad2, Pf2);
  }

  // Noise contribution from fourth corner
  vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
  o3 = o3 * ONE;
  float perm3xy = texture2D(perlinTexture, Pi.xy + o3.xy).a;
  float perm3zw = texture2D(perlinTexture, Pi.zw + o3.zw).a;
  vec4  grad3 = texture2D(gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if (t3 < 0.0) n3 = 0.0;
  else {
	t3 *= t3;
	n3 = t3 * t3 * dot(grad3, Pf3);
  }

  // Noise contribution from last corner
  vec4 Pf4 = Pf0 - vec4(1.0-4.0*G4);
  float perm4xy = texture2D(perlinTexture, Pi.xy + vec2(ONE, ONE)).a;
  float perm4zw = texture2D(perlinTexture, Pi.zw + vec2(ONE, ONE)).a;
  vec4  grad4 = texture2D(gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
  float t4 = 0.6 - dot(Pf4, Pf4);
  float n4;
  if(t4 < 0.0) n4 = 0.0;
  else {
	t4 *= t4;
	n4 = t4 * t4 * dot(grad4, Pf4);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

vec2 snoise4D2D(vec4 v)
{
	return vec2(snoise4D1D(vec4( v.x,  v.y,  v.z,  v.w)),
				snoise4D1D(vec4(-v.x, -v.y, -v.z, -v.w)));
}

vec3 snoise4D3D(vec4 v)
{
	return vec3(snoise4D1D(vec4(v.x-1, v.y-1, v.z-1, v.w-1)),
				snoise4D1D(vec4(v.x,   v.y,   v.z,   v.w)),
				snoise4D1D(vec4(v.x+1, v.y+1, v.z+1, v.w+1)));
}

vec4 snoise4D4D(vec4 v)
{
	return vec4(snoise4D1D(vec4(v.x-1, v.y-1, v.z-1, v.w-1)),
				snoise4D1D(vec4(v.x,   v.y,   v.z,   v.w)),
				snoise4D1D(vec4(v.x+1, v.y+1, v.z+1, v.w+1)),
				snoise4D1D(vec4(v.x+2, v.y+2, v.z+2, v.w+2)));
}
