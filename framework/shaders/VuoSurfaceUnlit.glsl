/**
 * Facilitates rendering an unlit surface.
 *
 * You just need to implement `void shadeSurface(inout Surface surface)`.
 *
 * This header includes a @c main() function,
 * which applies @c shadeSurface() to calculate the fragment color.
 */

struct Surface
{
	vec3 albedo;
	float alpha;
};

void shadeSurface(inout Surface surface);

void main()
{
	Surface surface;
	shadeSurface(surface);
	gl_FragColor = vec4(surface.albedo, surface.alpha);
}
