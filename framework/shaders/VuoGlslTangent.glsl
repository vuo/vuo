/**
 * @file
 * Calculates tangent and bitangent vectors for a triangle.
 */

// Structs, since GLSL doesn't support arrays as function arguments.
struct VuoGlslTangent_In
{
	vec3 position[3];
	vec3 normal[3];
	vec2 textureCoordinate[3];
};

struct VuoGlslTangent_Out
{
	vec3 tangent[3];
	vec3 bitangent[3];
};

void VuoGlsl_calculateTangent(in VuoGlslTangent_In ti, out VuoGlslTangent_Out to)
{
	// Based on "Computing Tangent Space Basis Vectors for an Arbitrary Mesh" by Eric Lengyel,
	// Terathon Software 3D Graphics Library, 2001.
	// https://web.archive.org/web/20160306000702/http://www.terathon.com/code/tangent.html

	vec3 tan1[3];
	tan1[0] = vec3(0);
	tan1[1] = vec3(0);
	tan1[2] = vec3(0);
	vec3 tan2[3];
	tan2[0] = vec3(0);
	tan2[1] = vec3(0);
	tan2[2] = vec3(0);

	vec3 v1 = ti.position[0];
	vec3 v2 = ti.position[1];
	vec3 v3 = ti.position[2];

	vec2 w1 = ti.textureCoordinate[0];
	vec2 w2 = ti.textureCoordinate[1];
	vec2 w3 = ti.textureCoordinate[2];

	float x1 = v2.x - v1.x;
	float x2 = v3.x - v1.x;
	float y1 = v2.y - v1.y;
	float y2 = v3.y - v1.y;
	float z1 = v2.z - v1.z;
	float z2 = v3.z - v1.z;

	float s1 = w2.x - w1.x;
	float s2 = w3.x - w1.x;
	float t1 = w2.y - w1.y;
	float t2 = w3.y - w1.y;

	float r = 1.0F / (s1 * t2 - s2 * t1);
	vec3 sdir = vec3(
		(t2 * x1 - t1 * x2) * r,
		(t2 * y1 - t1 * y2) * r,
		(t2 * z1 - t1 * z2) * r);
	vec3 tdir = vec3(
		(s1 * x2 - s2 * x1) * r,
		(s1 * y2 - s2 * y1) * r,
		(s1 * z2 - s2 * z1) * r);

	tan1[0] += sdir;
	tan1[1] += sdir;
	tan1[2] += sdir;

	tan2[0] += tdir;
	tan2[1] += tdir;
	tan2[2] += tdir;


	for (int i = 0; i < 3; ++i)
	{
		vec3 n = ti.normal[i];
		vec3 t = tan1[i];
		vec3 t2 = tan2[i];

		// Gram-Schmidt orthogonalize
		to.tangent[i]   = normalize(t  - n * dot(n, t));
		to.bitangent[i] = normalize(t2 - n * dot(n, t2));
	}
}
