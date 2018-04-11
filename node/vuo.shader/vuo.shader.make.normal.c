/**
 * @file
 * vuo.shader.make.color node implementation.
 * @todo Rename to vuo.shader.make.vertex or something, to reflect the new breadth of the shader.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoVertexAttribute.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Vertex Attribute Shader",
					 "keywords" : [ "mesh", "draw", "opengl", "glsl", "scenegraph", "graphics",
						 "colors", "direction", "heading", "facing",
						 "position", "location", "distance", "tangent", "bitangent", "texture coordinate"],
					 "version" : "1.3.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });


static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslProjection)

	// Inputs
	uniform mat4 modelviewMatrix;
	uniform int attr;
	uniform int local;
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	// Outputs to fragment shader
	varying vec3 vertexColor;

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
		if (attr == 1)
			vertexColor = normalize(abs(local > 0 ? normal.xyz : mat4to3(modelviewMatrix) * normal.xyz));
		else if (attr == 2)
			vertexColor = normalize(abs(local > 0 ? tangent.xyz : mat4to3(modelviewMatrix) * tangent.xyz));
		else if (attr == 3)
			vertexColor = normalize(abs(local > 0 ? bitangent.xyz : mat4to3(modelviewMatrix) * bitangent.xyz));
		else if (attr == 4 || attr == 5)
			vertexColor = textureCoordinate.xyz;
		else
			vertexColor = abs(local > 0 ? position : modelviewMatrix * position).xyz;

		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
	}
);

static const char *vertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslProjection)

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	uniform int attr;
	attribute vec4 position;
	attribute vec4 textureCoordinate;

	// Outputs to geometry shader
	varying vec4 positionForGeometry;
	varying vec4 textureCoordinateForGeometry;

	void main()
	{
		attr;
		positionForGeometry = modelviewMatrix * position;
		textureCoordinateForGeometry = textureCoordinate;
		gl_Position = VuoGlsl_projectPosition(positionForGeometry);
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
static const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from node
	uniform vec4 xColor;
	uniform vec4 yColor;
	uniform vec4 zColor;

	// Inputs from vertex shader
	varying vec3 vertexColor;

	void main()
	{
		vec4 c = mix(vec4(0.,0.,0.,1.), xColor, vertexColor.x)
			   + mix(vec4(0.,0.,0.,1.), yColor, vertexColor.y)
			   + mix(vec4(0.,0.,0.,1.), zColor, vertexColor.z);
		c = clamp(c, 0., 1.);
		gl_FragColor = c;
	}
);

static const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from node
	uniform int attr;
	uniform vec4 xColor;
	uniform vec4 yColor;
	uniform vec4 zColor;

	// Inputs from geometry shader
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = xColor;

		fragmentTextureCoordinate;

		vec3 vertexColor;
		if (attr == 1)	// normal
			vertexColor = normalize(abs(vertexPlaneToWorld[2]));
		else if (attr == 2)	// tangent
			vertexColor = normalize(abs(vertexPlaneToWorld[0]));
		else if (attr == 3)	// bitangent
			vertexColor = normalize(abs(vertexPlaneToWorld[1]));
		else if (attr == 5)
			vertexColor = fragmentTextureCoordinate.xyz;
		else
			vertexColor = abs(vertexPosition.xyz);

		vec4 c = mix(vec4(0.,0.,0.,1.), xColor, vertexColor.x)
			   + mix(vec4(0.,0.,0.,1.), yColor, vertexColor.y)
			   + mix(vec4(0.,0.,0.,1.), zColor, vertexColor.z);
		c = clamp(c, 0., 1.);
		gl_FragColor = c;
	}
);

static const char *checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform vec4 xColor;
	uniform vec4 yColor;
	varying vec3 vertexColor;

	void main()
	{
		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec3 color0 = xColor.rgb * vertexColor.x;
		vec3 color1 = yColor.rgb * vertexColor.y;
		float frequency = 8;
		vec2 filterWidth = fwidth(vertexColor.xy) * frequency;

		vec2 checkPos = fract(vertexColor.xy * frequency);
		vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
			(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

		gl_FragColor = vec4(mix(color0, color1, p.x*p.y + (1-p.x)*(1-p.y)), 1);
	}
);

static const char *checkerboardFragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform vec4 xColor;
	uniform vec4 yColor;
	varying vec4 fragmentTextureCoordinate;
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		vertexPosition;
		vertexPlaneToWorld;

		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec3 color0 = xColor.rgb * fragmentTextureCoordinate.x;
		vec3 color1 = yColor.rgb * fragmentTextureCoordinate.y;
		float frequency = 8;
		vec2 filterWidth = fwidth(fragmentTextureCoordinate.xy) * frequency;

		vec2 checkPos = fract(fragmentTextureCoordinate.xy * frequency);
		// Add 0.00001 so that smoothstep() doesn't return NaN on ATI Radeon HD 5770.
		// https://b33p.net/kosada/node/10467
		vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
			(1 - smoothstep(vec2(0),   filterWidth,             checkPos+0.00001));

		gl_FragColor = vec4(mix(color0, color1, p.x*p.y + (1-p.x)*(1-p.y)), 1);
	}
);


void nodeEvent
(
		VuoInputData(VuoVertexAttribute, {"default":"normal"}) attribute,
		VuoInputData(VuoInteger, { "menuItems":{
			"0":"World",
			"1":"Local"
		}, "default":0} ) coordinateSpace,
		VuoInputData(VuoColor, {"default":{"r":1.0,"g":0.0,"b":0.0,"a":1.0}}) xColor,
		VuoInputData(VuoColor, {"default":{"r":0.0,"g":1.0,"b":0.0,"a":1.0}}) yColor,
		VuoInputData(VuoColor, {"default":{"r":0.0,"g":0.0,"b":1.0,"a":1.0}}) zColor,
		VuoOutputData(VuoShader) shader
)
{
	const char *fs  = (attribute == VuoVertexAttribute_TextureCoordinateChecker) ? checkerboardFragmentShaderSource            : fragmentShaderSource;
	const char *fsg = (attribute == VuoVertexAttribute_TextureCoordinateChecker) ? checkerboardFragmentShaderSourceForGeometry : fragmentShaderSourceForGeometry;

	*shader = VuoShader_make("Vertex Attribute Shader");

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSourceForGeometry, pointGeometryShaderSource, fsg);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSourceForGeometry, lineGeometryShaderSource,  fsg);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSource,            NULL,                      fs);

	VuoShader_setUniform_VuoInteger(*shader, "attr", attribute);
	VuoShader_setUniform_VuoInteger(*shader, "local", coordinateSpace);
	VuoShader_setUniform_VuoColor(*shader, "xColor", xColor);
	VuoShader_setUniform_VuoColor(*shader, "yColor", yColor);
	if (attribute != VuoVertexAttribute_TextureCoordinateChecker
	 && attribute != VuoVertexAttribute_TextureCoordinateGradient)
		VuoShader_setUniform_VuoColor(*shader, "zColor", zColor);
}
