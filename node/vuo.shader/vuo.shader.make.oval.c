/**
 * @file
 * vuo.shader.make.oval node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Oval Shader",
					  "keywords" : [
						  "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "point", "star", "circle", "rounded", "ellipse", "disc", "disk", "dot",
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "vuo-example://vuo.scene/TileStarfield.vuo" ]
					  }
				  });

static const char *defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslProjection.glsl"

	// Inputs
	uniform mat4 modelviewMatrix;
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;
	uniform bool hasVertexColors;

	// Outputs to fragment shader
	varying vec3 fragmentPosition;
	varying vec3 fragmentNormal;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main()
	{
		fragmentPosition = (modelviewMatrix * vec4(position, 1.)).xyz;
		fragmentNormal   = (modelviewMatrix * vec4(normal, 0.)).xyz;
		fragmentTextureCoordinate = textureCoordinate;
		fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(fragmentPosition);
	}
);

static const char *defaultVertexShaderSourceForGeometryShader = VUOSHADER_GLSL_SOURCE(120,
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
		geometryPosition = (cameraMatrixInverse * modelviewMatrix * vec4(position, 1.)).xyz;
		geometryNormal = normal;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
static const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform vec4 ovalColor;
	uniform float ovalWidth;
	uniform float ovalSharpness;
	uniform vec4 haloColor;

	varying vec3 fragmentPosition;
	varying vec3 fragmentNormal;
	varying vec2 fragmentTextureCoordinate;
	varying vec4 fragmentVertexColor;

	void main(void)
	{
		fragmentPosition;
		fragmentNormal;

		float fw = fwidth(fragmentTextureCoordinate.x);
		float sharp = max(0., ovalSharpness - fw);
		float width = ovalWidth + fw*2.;

		float dist = distance(fragmentTextureCoordinate, vec2(0.5,0.5)) * 2.;
		float ovalDist = dist / width;
		float delta = fwidth(ovalDist)/2.;
		vec4 c = mix(ovalColor * fragmentVertexColor, vec4(0.), smoothstep(sharp/2. - delta, 1. - sharp/2. + delta, ovalDist) + fw*2.)
			   + mix(haloColor * fragmentVertexColor, vec4(0.), pow(smoothstep(0., 1., dist * .93), .01)) * 10.;
		gl_FragColor = clamp(c, 0., 1.);
	}
);

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) ovalColor,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":2}) ovalWidth,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0, "suggestedMax":1}) ovalSharpness,
		VuoInputData(VuoColor,{"default":{"r":0.1,"g":0.2,"b":1,"a":1}}) haloColor,
		VuoOutputData(VuoShader) shader
)
{

	*shader = VuoShader_make("Oval Shader (with halo)");
//	(*shader)->objectScale = 0.5;
	(*shader)->isTransparent = true;

	VuoShader_addSource                      ((*shader), VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount((*shader), VuoMesh_Points, 2);

	VuoShader_addSource                      ((*shader), VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount((*shader), VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      ((*shader), VuoMesh_IndividualTriangles, defaultVertexShaderSource,                  NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor((*shader), "ovalColor",     ovalColor);
	VuoShader_setUniform_VuoReal ((*shader), "ovalWidth",      VuoReal_makeNonzero(ovalWidth));
	VuoShader_setUniform_VuoReal ((*shader), "ovalSharpness", VuoReal_clamp(ovalSharpness, 0, 1));
	VuoShader_setUniform_VuoColor((*shader), "haloColor",     haloColor);
}
