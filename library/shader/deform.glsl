/**
 * Facilitates defining a GLSL vertex deformation shader.
 *
 * You just need to implement `vec3 deform(vec3 position)`.
 *
 * This header includes a @c main() function,
 * which applies @c deform() to calculate the vertex position,
 * then applies it to neighboring positions on the plane tangent
 * to the surface at the current vertex, in order to estimate
 * the deformed normal, tangent, and bitangent.
 */

uniform mat4 modelviewMatrix;
uniform mat4 modelviewMatrixInverse;

attribute vec4 position;
attribute vec4 normal;
attribute vec4 tangent;
attribute vec4 bitangent;
attribute vec4 textureCoordinate;

varying vec4 outPosition;
varying vec4 outNormal;
varying vec4 outTangent;
varying vec4 outBitangent;
varying vec4 outTextureCoordinate;

vec3 deform(vec3 position);

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
	// Position ============================================================

	// Transform into worldspace.
	vec4 positionInScene = modelviewMatrix * position;

	// Apply the deformation.
	vec3 deformedPosition = deform(positionInScene.xyz);

	// Transform back into modelspace.
	outPosition = modelviewMatrixInverse * vec4(deformedPosition, 1.);


	// Normal/Tangent/Bitangent ============================================

	// Transform the tangent and bitangent into worldspace.
	// Since these are directional vectors, use the top 3x3 of the modelviewMatrix to apply just the rotation/scale (not the translation).
	mat3 modelviewMatrix3 = mat4to3(modelviewMatrix);
	vec3 tangentInScene   = modelviewMatrix3 *   tangent.xyz;
	vec3 bitangentInScene = modelviewMatrix3 * bitangent.xyz;

	// Apply the deformation to neighboring positions.
	vec3 deformedAlongTangent   = deform(positionInScene.xyz +   tangentInScene/100.);
	vec3 deformedAlongBitangent = deform(positionInScene.xyz + bitangentInScene/100.);

	// Calculate the orthonormal basis of the tangent plane at deformedPosition.
	vec3   tangent = deformedAlongTangent   - deformedPosition;
	vec3 bitangent = deformedAlongBitangent - deformedPosition;
	vec3    normal = cross(tangent, bitangent);

	// Transform back into modelspace.
	mat3 modelviewMatrix3i = mat4to3(modelviewMatrixInverse);
	outNormal    = vec4(normalize(modelviewMatrix3i *    normal), 1.);
	outTangent   = vec4(normalize(modelviewMatrix3i *   tangent), 1.);
	outBitangent = vec4(normalize(modelviewMatrix3i * bitangent), 1.);


	// Texture Coordinate ==================================================
	outTextureCoordinate = textureCoordinate;


	// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
	// fall back to the software renderer if gl_Position is not initialized.
	gl_Position = vec4(0);
}
