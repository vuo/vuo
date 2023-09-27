/**
 * Projects 3D points from worldspace into clipspace.
 *
 * In your vertex shader, call this as follows: `gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);`
 */

// Inputs provided by VuoSceneRenderer
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrixInverse;
uniform bool useFisheyeProjection;

const float PI_DIV_2 = 1.570796327;

vec4 VuoGlsl_projectPosition(vec3 position)
{
	if (!useFisheyeProjection)
		// Standard linear projection.
		return projectionMatrix * cameraMatrixInverse * vec4(position, 1.);
	else
	{
		// Hemispherical fisheye projection, based on "Realtime Dome Imaging and Interaction" by Bailey/Clothier/Gebbie 2006.
		// https://web.archive.org/web/20090116013458/http://web.engr.oregonstate.edu/~mjb/WebMjb/Papers/asmedome.pdf

		vec3 pos = (cameraMatrixInverse * vec4(position, 1.)).xyz;
		float rxy = length(pos.xy);

		if (rxy != 0.)
		{
			// Work around Intel GPU bug --- using the negation operator on an atan() argument causes the function to be specialized to integer (not float, which we need here).
			// https://b33p.net/kosada/node/10911
			float negativeZ = -pos.z;

			// The unit circle suggests that when x=0, the inverse tangent for positive y values should be π/2.
			// However, the GLSL spec says "The result is undefined if x=0",
			// and the macOS GLSL implementation, at least, returns -π/2 (i.e. flipped to the opposite side of the circle).
			// Instead, use the geometically-expected value.
			// This fixes flickering shards that appeared when triangles crossed the camera ground plane.
			// https://b33p.net/kosada/vuo/vuo/-/issues/19583#note_2188920
			float phi = pos.z == 0 ? PI_DIV_2 : atan(rxy, negativeZ);

			float lens_radius = phi / PI_DIV_2;

			pos.xy *= lens_radius / rxy;
		}

		return projectionMatrix * vec4(pos, 1.);
	}
}

vec4 VuoGlsl_projectPosition(vec4 position)
{
	return VuoGlsl_projectPosition(position.xyz);
}
