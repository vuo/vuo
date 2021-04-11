/**
 * @file
 * vuo.shader.make.color node implementation.
 * @todo Rename to vuo.shader.make.vertex or something, to reflect the new breadth of the shader.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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


static const char *vertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslProjection.glsl"

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	uniform int local;
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	// Outputs to geometry shader
	varying vec3 geometryPosition;
	varying vec3 geometryNormal;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

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
		geometryPosition = local > 0 ? position : (modelviewMatrix * vec4(position, 1.)).xyz;
		geometryNormal = local > 0 ? normal : mat4to3(modelviewMatrix) * normal;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = vertexColor;
		gl_Position = VuoGlsl_projectPosition((modelviewMatrix * vec4(position, 1.)).xyz);
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
static const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

static const char *triangleGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslTangent.glsl"

	// Inputs
	varying in vec3 geometryPosition[3];
	varying in vec3 geometryNormal[3];
	varying in vec2 geometryTextureCoordinate[3];
	varying in vec4 geometryVertexColor[3];

	// Outputs
	varying out vec3 fragmentPosition;
	varying out vec3 fragmentNormal;
	varying out vec3 fragmentTangent;
	varying out vec3 fragmentBitangent;
	varying out vec2 fragmentTextureCoordinate;
	varying out vec4 fragmentVertexColor;

	void main()
	{
		VuoGlslTangent_In ti;
		for (int i = 0; i < 3; ++i)
		{
			ti.position[i] = geometryPosition[i];
			ti.normal[i] = geometryNormal[i];
			ti.textureCoordinate[i] = geometryTextureCoordinate[i];
		}
		VuoGlslTangent_Out to;
		VuoGlsl_calculateTangent(ti, to);

		for (int i = 0; i < 3; ++i)
		{
			gl_Position = gl_PositionIn[i];
			fragmentPosition = geometryPosition[i];
			fragmentNormal = geometryNormal[i];
			fragmentTangent = to.tangent[i];
			fragmentBitangent = to.bitangent[i];
			fragmentTextureCoordinate = geometryTextureCoordinate[i];
			fragmentVertexColor = geometryVertexColor[i];
			EmitVertex();
		}
		EndPrimitive();
	}
);

static const char *triangleFragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslTangent.glsl"

	// Inputs from node
	uniform int attr;
	uniform vec4 xColor;
	uniform vec4 yColor;
	uniform vec4 zColor;

	// Inputs from geometry shader
	varying vec3 fragmentPosition;
	varying vec3 fragmentNormal;
	varying vec3 fragmentTangent;
	varying vec3 fragmentBitangent;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = xColor;

		fragmentTextureCoordinate;
		fragmentVertexColor;

		vec3 vertexColor;
		if (attr == 1)	// normal
			vertexColor = normalize(abs(fragmentNormal));
		else if (attr == 2)	// tangent
			vertexColor = normalize(abs(fragmentTangent));
		else if (attr == 3)	// bitangent
			vertexColor = normalize(abs(fragmentBitangent));
		else if (attr == 5)  // VuoVertexAttribute_TextureCoordinateGradient
			vertexColor = vec3(fragmentTextureCoordinate, 0.);
		else
			vertexColor = abs(fragmentPosition);

		vec4 c = mix(vec4(0.,0.,0.,1.), xColor, vertexColor.x)
			   + mix(vec4(0.,0.,0.,1.), yColor, vertexColor.y)
			   + mix(vec4(0.,0.,0.,1.), zColor, vertexColor.z);
		c = clamp(c, 0., 1.);
		gl_FragColor = c;
	}
);

static const char *pointLineFragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from node
	uniform int attr;
	uniform vec4 xColor;
	uniform vec4 yColor;
	uniform vec4 zColor;

	// Inputs from geometry shader
	varying vec3 fragmentPosition;
	varying vec3 fragmentNormal;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = xColor;

		fragmentTextureCoordinate;
		fragmentVertexColor;

		vec3 vertexColor;
		if (attr == 1)  // normal
			vertexColor = normalize(abs(fragmentNormal));
		else if (attr == 2)  // tangent
			vertexColor = vec3(1,0,0);
		else if (attr == 3)  // bitangent
			vertexColor = vec3(0,1,0);
		else if (attr == 5)  // VuoVertexAttribute_TextureCoordinateGradient
			vertexColor = vec3(fragmentTextureCoordinate, 0.);
		else
			vertexColor = abs(fragmentPosition);

		vec4 c = mix(vec4(0.,0.,0.,1.), xColor, vertexColor.x)
			+ mix(vec4(0.,0.,0.,1.), yColor, vertexColor.y)
			+ mix(vec4(0.,0.,0.,1.), zColor, vertexColor.z);
		c = clamp(c, 0., 1.);
		gl_FragColor = c;
	}
	);

static const char *checkerboardFragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform int attr;
	uniform vec4 xColor;
	uniform vec4 yColor;
	varying vec3 fragmentPosition;
	varying vec3 fragmentNormal;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main()
	{
		attr;
		fragmentPosition;
		fragmentNormal;
		fragmentVertexColor;

		// Based on the Gritz/Baldwin antialiased checkerboard shader.

		vec3 color0 = xColor.rgb * fragmentTextureCoordinate.x;
		vec3 color1 = yColor.rgb * fragmentTextureCoordinate.y;
		float frequency = 8;
		vec2 filterWidth = fwidth(fragmentTextureCoordinate) * frequency;

		vec2 checkPos = fract(fragmentTextureCoordinate * frequency);
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
		VuoInputData(VuoInteger, { "menuItems":[
			{"value":0, "name":"World"},
			{"value":1, "name":"Local"},
		], "default":0} ) coordinateSpace,
		VuoInputData(VuoColor, {"default":{"r":1.0,"g":0.0,"b":0.0,"a":1.0}}) xColor,
		VuoInputData(VuoColor, {"default":{"r":0.0,"g":1.0,"b":0.0,"a":1.0}}) yColor,
		VuoInputData(VuoColor, {"default":{"r":0.0,"g":0.0,"b":1.0,"a":1.0}}) zColor,
		VuoOutputData(VuoShader) shader
)
{
	const char *plFsg = (attribute == VuoVertexAttribute_TextureCoordinateChecker) ? checkerboardFragmentShaderSourceForGeometry : pointLineFragmentShaderSourceForGeometry;
	const char *tFsg  = (attribute == VuoVertexAttribute_TextureCoordinateChecker) ? checkerboardFragmentShaderSourceForGeometry : triangleFragmentShaderSourceForGeometry;

	*shader = VuoShader_make("Vertex Attribute Shader");

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSourceForGeometry, pointGeometryShaderSource, plFsg);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSourceForGeometry, lineGeometryShaderSource,  plFsg);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSourceForGeometry, triangleGeometryShaderSource, tFsg);

	VuoShader_setUniform_VuoInteger(*shader, "attr", attribute);
	VuoShader_setUniform_VuoInteger(*shader, "local", coordinateSpace);
	VuoShader_setUniform_VuoColor(*shader, "xColor", xColor);
	VuoShader_setUniform_VuoColor(*shader, "yColor", yColor);
	if (attribute != VuoVertexAttribute_TextureCoordinateChecker
	 && attribute != VuoVertexAttribute_TextureCoordinateGradient)
		VuoShader_setUniform_VuoColor(*shader, "zColor", zColor);
}
