/**
 * @file
 * vuo.shader.make.edge.color node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Edge Shader",
					  "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "rim", "glow", "ghost", "trace", "cartoon" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "CompareColorAndEdgeShaders.vuo", "MeetTheGhostOfEinstein.vuo" ]
					  }
				  });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslProjection.glsl"

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec4 vertexColor;
	uniform bool hasVertexColors;

	// Outputs to fragment shader
	varying vec3 projectedNormal;
	varying vec4 fragmentVertexColor;

	mat3 mat4to3(mat4 m)
	{
		return mat3(
			m[0][0], m[0][1], m[0][2],
			m[1][0], m[1][1], m[1][2],
			m[2][0], m[2][1], m[2][2]);
	}

	void main()
	{
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));

		projectedNormal = mat4to3(modelviewMatrix) * normal;
		fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);
	}
);

static const char *vertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslProjection.glsl"

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;
	uniform bool hasVertexColors;

	// Outputs to geometry shader
	varying vec3 geometryPosition;
	varying vec3 geometryNormal;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

	void main()
	{
		geometryPosition = (modelviewMatrix * vec4(position, 1.)).xyz;
		geometryNormal = (modelviewMatrix * vec4(normal, 0.)).xyz;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(geometryPosition);
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
static const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform vec3 cameraPosition;

	// Inputs from ports
	uniform vec4 color;
	uniform float width;
	uniform float sharpness;
	uniform float interiorOpacity;

	// Inputs from vertex shader
	varying vec3 projectedNormal;
	varying vec4 fragmentVertexColor;

	void main()
	{
		float d = dot(normalize(projectedNormal), normalize(cameraPosition));
		d = gl_FrontFacing ? - d : d;
		d = smoothstep(sharpness, 1.0000001 - sharpness, width + d);
		gl_FragColor = mix(
					vec4(0.),
					vec4(color.rgb * fragmentVertexColor.rgb, 1.),
					d * color.a * fragmentVertexColor.a + (1. - d) * interiorOpacity);
	}
);

static const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform vec3 cameraPosition;

	// Inputs from ports
	uniform vec4 color;
	uniform float width;
	uniform float sharpness;
	uniform float interiorOpacity;

	// Inputs from geometry shader
	varying vec3 fragmentPosition;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = color;

		fragmentPosition;

		float x = fragmentTextureCoordinate.x;
		float y = fragmentTextureCoordinate.y;

		// Increase the edgeWidth if the texture coordinates are changing very quickly (i.e., if the point/line is very small), to reduce aliasing.
		float delta = fwidth(x) + fwidth(y);
		float w = width/4.;

		float dx = smoothstep(sharpness-delta/2., 1.0000001 - sharpness + delta/2., x + w - 0.5)
				 + smoothstep(1.0000001 - sharpness + delta/2.,  sharpness - delta/2., x - w + 0.5);
		float dy = smoothstep(sharpness-delta/2., 1.0000001 - sharpness + delta/2., y + w - 0.5)
				 + smoothstep(1.0000001 - sharpness + delta/2.,  sharpness - delta/2., y - w + 0.5);
		float d = min(1., sqrt(pow(dx,2.) + pow(dy,2.)));
		gl_FragColor = mix(
					vec4(0.),
					vec4(color.rgb * fragmentVertexColor.rgb, 1.),
					d * color.a * fragmentVertexColor.a + (1. - d) * interiorOpacity);
	}
);

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1}) width,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1}) sharpness,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) interiorOpacity,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("Shade Edges with Color");
	(*shader)->isTransparent = true;

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSource, NULL,      fragmentShaderSource);

	// Use VuoPoint4d instead of VuoColor since the shader can more efficiently work with a straight (un-premultiplied) color.
	VuoShader_setUniform_VuoPoint4d(*shader, "color", (VuoPoint4d){color.r, color.g, color.b, color.a});

	VuoShader_setUniform_VuoReal (*shader, "width",           width*2.);
	VuoShader_setUniform_VuoReal (*shader, "sharpness",       MAX(0.,MIN(1.,sharpness)) / 2.);
	VuoShader_setUniform_VuoReal (*shader, "interiorOpacity", interiorOpacity);
}
