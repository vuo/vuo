/**
 * Improved canonical pseudorandomness (minus the `highp` improvement which isn't supported on GLSL 1.20).
 *
 * Each function returns values between 0 and 1.
 *
 * http://web.archive.org/web/20141130173831/http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0
 */

float VuoGlsl_random1D1D(float co)
{
	float a = 12.9898;
	float c = 43758.5453;
	float dt= co*a;
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

float VuoGlsl_random2D1D(vec2 co)
{
	float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt= dot(co,vec2(a,b));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

float VuoGlsl_random3D1D(vec3 co)
{
	float c = 43758.5453;
	float dt= dot(co,vec3(12.9898,78.233,469.398));
	float sn= mod(dt,3.14);
	return fract(sin(sn) * c);
}

vec3 VuoGlsl_random2D3D(vec2 v)
{
	return vec3(VuoGlsl_random2D1D(vec2(v.x-1, v.y-1)),
				VuoGlsl_random2D1D(vec2(v.x,   v.y)),
				VuoGlsl_random2D1D(vec2(v.x+1, v.y+1)));
}

vec3 VuoGlsl_random3D3D(vec3 v)
{
	return vec3(VuoGlsl_random3D1D(vec3(v.x-1, v.y-1, v.z-1)),
				VuoGlsl_random3D1D(vec3(v.x,   v.y,   v.z)),
				VuoGlsl_random3D1D(vec3(v.x+1, v.y+1, v.z+1)));
}
