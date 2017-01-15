/**
 * @file
 * vuo.shader.make.wireframe node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Wireframe",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						 "outline", "edges", "lines", "skeletal" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SpinWireframeSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslProjection)

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
//	attribute vec4 textureCoordinate;

	// Outputs to geometry shader
//	varying vec2 textureCoordinateForGeometry;

	void main()
	{
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
//		textureCoordinateForGeometry = textureCoordinate.xy;
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from vertex shader
//	varying in vec2 textureCoordinateForGeometry[3];

	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;
//	varying out vec2 geometryTextureCoordinate;

	void main()
	{
		gl_Position = gl_PositionIn[0];
		geometryDistanceFromEdge = vec3(1,0,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();

		gl_Position = gl_PositionIn[1];
		geometryDistanceFromEdge = vec3(0,1,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[1];
		EmitVertex();

		gl_Position = gl_PositionIn[2];
		geometryDistanceFromEdge = vec3(0,0,1);
//		geometryTextureCoordinate = textureCoordinateForGeometry[2];
		EmitVertex();

		EndPrimitive();
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from VuoSceneRenderer
	uniform float aspectRatio;
	uniform float primitiveHalfSize;

	// Inputs from vertex shader
//	varying in vec2 textureCoordinateForGeometry[1];

	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;
//	varying out vec2 geometryTextureCoordinate;

	void main()
	{
		vec2 pointSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(1,0,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,1,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,0,1);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		EndPrimitive();

		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(1,0,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,1,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,0,1);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		EndPrimitive();
	}
);

static const char *lineGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform float aspectRatio;
	uniform float primitiveHalfSize;
	uniform vec3 cameraPosition;
	uniform bool useFisheyeProjection;

	// Inputs from vertex shader
//	varying in vec2 textureCoordinateForGeometry[2];

	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;
//	varying out vec2 geometryTextureCoordinate;

	void main()
	{
		vec2 lineSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

		vec3 cameraPosition = useFisheyeProjection ? vec3(0,0,-1000) : (projectionMatrix * vec4(cameraPosition,1)).xyz;

		// Screen-space direction perpendicular to the line segment.
		vec3 perpendicular = normalize(
					cross(gl_PositionIn[1].xyz - gl_PositionIn[0].xyz,
						  gl_PositionIn[0].xyz - cameraPosition)
				);

		vec4 perpendicularOffset = vec4(perpendicular.x*lineSize.x, perpendicular.y*lineSize.y, 0, 0);

		gl_Position               = gl_PositionIn[1]       - perpendicularOffset;
		geometryDistanceFromEdge  = vec3(1,0,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[1];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       - perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,1,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,0,1);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		EndPrimitive();

		gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(1,0,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[0];
		EmitVertex();
		gl_Position               = gl_PositionIn[1]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,1,0);
//		geometryTextureCoordinate = textureCoordinateForGeometry[1];
		EmitVertex();
		gl_Position               = gl_PositionIn[1]       - perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,0,1);
//		geometryTextureCoordinate = textureCoordinateForGeometry[1];
		EmitVertex();
		EndPrimitive();
	}
);

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from ports
	uniform vec4 frontLineColor;
	uniform vec4 backLineColor;
	uniform float width;
	uniform float standoffWidth;
	uniform bool showThirdEdge;

	// Inputs from geometry shader
	varying vec3 geometryDistanceFromEdge;
//	varying vec2 geometryTextureCoordinate;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = frontLineColor;

//		vec2 filterWidth = fwidth(geometryTextureCoordinate);
//		float maximumFilterWidth = max(filterWidth.x, filterWidth.y) * 8;

		float minimumDistanceFromEdge = min(geometryDistanceFromEdge.x, geometryDistanceFromEdge.z);
		if (showThirdEdge)
			minimumDistanceFromEdge = min(minimumDistanceFromEdge, geometryDistanceFromEdge.y);

		if (minimumDistanceFromEdge > width+standoffWidth)
			discard;

//		float interiorness = smoothstep(width-maximumFilterWidth, width+maximumFilterWidth, minimumDistanceFromEdge);
//		gl_FragColor = mix(gl_FrontFacing?frontLineColor:backLineColor, vec4(0,0,0,1), interiorness);
		gl_FragColor = minimumDistanceFromEdge < width ? (gl_FrontFacing?frontLineColor:backLineColor) : vec4(0,0,0,1);
	}
);

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":0.6,"g":0.8,"b":0.6,"a":1}}) frontColor,
		VuoInputData(VuoColor, {"default":{"r":0.3,"g":0.4,"b":0.3,"a":1}}) backColor,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.05}) width,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.05}) standoffWidth,
		VuoInputData(VuoBoolean, {"default":true}) showThirdEdge,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("Wireframe Shader");

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSource, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSource, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSource, geometryShaderSource,      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(*shader, "frontLineColor", frontColor);
	VuoShader_setUniform_VuoColor(*shader, "backLineColor", backColor);
	VuoShader_setUniform_VuoReal(*shader, "width", width/2.);
	VuoShader_setUniform_VuoReal(*shader, "standoffWidth", standoffWidth/2.);
	VuoShader_setUniform_VuoBoolean(*shader, "showThirdEdge", showThirdEdge);
}
