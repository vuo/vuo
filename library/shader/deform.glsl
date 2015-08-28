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

void main()
{
	// Transform into worldspace.
	vec4 positionInScene = modelviewMatrix * position;

	// Apply the deformation.
	vec3 deformedPositionInScene = deform(positionInScene.xyz);

	// Transform back into modelspace.
	outPosition = modelviewMatrixInverse * vec4(deformedPositionInScene,1);

	vec3 deformedPosition       = deform(position.xyz);
	vec3 deformedAlongTangent   = deform(position.xyz + tangent.xyz/100);
	vec3 deformedAlongBitangent = deform(position.xyz + bitangent.xyz/100);
	vec3 ab = deformedAlongTangent   - deformedPosition;
	vec3 ac = deformedAlongBitangent - deformedPosition;
	outNormal    = vec4(normalize(cross(ab, ac)),1);
	outTangent   = vec4(normalize(ab),1);
	outBitangent = vec4(normalize(ac),1);

	outTextureCoordinate = textureCoordinate;

	// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
	// fall back to the software renderer if gl_Position is not initialized.
	gl_Position = vec4(0);
}
