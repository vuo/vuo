/**
 * Facilitates defining a GLSL vertex deformation shader.
 *
 * You just need to implement `vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)`.
 *
 * This header includes a @c main() function,
 * which applies @c deform() to calculate the vertex position,
 * then applies it to neighboring positions on the plane tangent
 * to the surface at the current vertex, in order to estimate
 * the deformed normal, tangent, and bitangent.
 */

uniform mat4 modelviewMatrix;
uniform mat4 modelviewMatrixInverse;

attribute vec3 position;
attribute vec3 normal;
attribute vec2 textureCoordinate;
attribute vec4 vertexColor;

varying vec3 outPosition;
varying vec3 outNormal;
varying vec2 outTextureCoordinate;
varying vec4 outVertexColor;

vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate);

// Can't simply use `mat3(mat4 m)` since it doesn't work on NVIDIA GeForce 9400M on Mac OS 10.6.
mat3 mat4to3(mat4 m)
{
	return mat3(
		m[0][0], m[0][1], m[0][2],
		m[1][0], m[1][1], m[1][2],
		m[2][0], m[2][1], m[2][2]);
}

void main()
{
	// Keep this in sync with VuoSceneObjectRenderer_makeDeformer.

	// Position ============================================================

	// Transform into worldspace.
	vec3 positionInScene = (modelviewMatrix * vec4(position, 1.)).xyz;

	mat3 modelviewMatrix3 = mat4to3(modelviewMatrix);
	vec3 normalInScene = normalize(modelviewMatrix3 * normal);

	// Apply the deformation.
	vec3 deformedPosition = deform(positionInScene, normalInScene, textureCoordinate);

	// Transform back into modelspace.
	outPosition = (modelviewMatrixInverse * vec4(deformedPosition, 1.)).xyz;


	// Normal ==============================================================

	// Pick an arbitrary tangent (the better of two arbitrary vectors).
	vec3 t0 = cross(normalInScene, vec3(0, 0, -1));
	vec3 t1 = cross(normalInScene, vec3(0, -1, 0));
	vec3 tangentInScene = length(t0) > length(t1) ? t0 : t1;

	vec3 bitangentInScene = cross(tangentInScene, normalInScene);

	// Apply the deformation to neighboring positions.
	vec3 deformedAlongTangent   = deform(positionInScene.xyz +   tangentInScene/100.,
										 normalInScene,
										 textureCoordinate.xy + vec2(.01, 0.));
	vec3 deformedAlongBitangent = deform(positionInScene.xyz + bitangentInScene/100.,
										 normalInScene,
										 textureCoordinate.xy + vec2(0., .01));

	// Calculate the orthonormal basis of the tangent plane at deformedPosition.
	vec3   tangent = deformedAlongTangent   - deformedPosition;
	vec3 bitangent = deformedAlongBitangent - deformedPosition;
	vec3    normal = cross(bitangent, tangent);

	// Transform back into modelspace.
	mat3 modelviewMatrix3i = mat4to3(modelviewMatrixInverse);
	outNormal = normalize(modelviewMatrix3i * normal);


	// Pass other attributes through =======================================
	outTextureCoordinate = textureCoordinate;
	outVertexColor = vertexColor;


	// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
	// fall back to the software renderer if gl_Position is not initialized.
	gl_Position = vec4(0);
}
