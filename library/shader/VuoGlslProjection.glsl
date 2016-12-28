/**
 * Projects 3D points from worldspace into clipspace.
 *
 * In your vertex shader, call this as follows: `gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);`
 */

// Inputs provided by VuoSceneRenderer
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrixInverse;
uniform bool useFisheyeProjection;

vec4 VuoGlsl_projectPosition(vec4 position)
{
	if (!useFisheyeProjection)
		// Standard linear projection.
		return projectionMatrix * cameraMatrixInverse * position;
	else
	{
		// Hemispherical fisheye projection, based on "Realtime Dome Imaging and Interaction" by Bailey/Clothier/Gebbie 2006.

		float phi;
		vec4 pos = cameraMatrixInverse * position;
		float rxy = length(pos.xy);

		if (rxy != 0.)
		{
			// Work around Intel GPU bug --- using the negation operator on an atan() argument causes the function to be specialized to integer (not float, which we need here).
			// https://b33p.net/kosada/node/10911
			float negativeZ = -pos.z;

			float phi = atan(rxy, negativeZ);
			float lens_radius = phi / 1.570796 /* PI/2 */;
			pos.xy *= lens_radius / rxy;
		}

		// Cut down on z-fighting near the edges of the circle.
		pos.z *= 10.;

		return projectionMatrix * pos;
	}
}
