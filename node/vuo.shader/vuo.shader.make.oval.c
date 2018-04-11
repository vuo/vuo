/**
 * @file
 * vuo.shader.make.oval node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Shade with Oval",
					  "keywords" : [
						  "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "point", "star", "circle", "rounded", "ellipse", "disc", "disk",
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "vuo-example://vuo.scene/TileStarfield.vuo" ]
					  }
				  });

static const char *defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslProjection)

	// Inputs
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 textureCoordinate;

	// Outputs to fragment shader
	varying vec4 fragmentTextureCoordinate;
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		fragmentTextureCoordinate = textureCoordinate;
		vertexPosition = vec4(0.);
		vertexPlaneToWorld = mat3(0.);

		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
	}
);

static const char *defaultVertexShaderSourceForGeometryShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslProjection)

	// Inputs provided by VuoSceneRenderer
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 textureCoordinate;

	// Outputs to geometry shader
	varying vec4 positionForGeometry;
	varying vec4 textureCoordinateForGeometry;

	void main()
	{
		positionForGeometry = cameraMatrixInverse * modelviewMatrix * position;
		textureCoordinateForGeometry = textureCoordinate;
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
static const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform vec4 ovalColor;
	uniform float ovalWidth;
	uniform float ovalSharpness;
	uniform vec4 haloColor;

	varying vec4 fragmentTextureCoordinate;
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;

	void main(void)
	{
		vertexPosition;
		vertexPlaneToWorld;

		float fw = fwidth(fragmentTextureCoordinate.x);
		float sharp = max(0., ovalSharpness - fw);
		float width = ovalWidth + fw*2.;

		float dist = distance(fragmentTextureCoordinate.xy, vec2(0.5,0.5)) * 2.;
		float ovalDist = dist / width;
		float delta = fwidth(ovalDist)/2.;
		vec4 c = mix(ovalColor, vec4(0.), smoothstep(sharp/2. - delta, 1. - sharp/2. + delta, ovalDist) + fw*2.)
			   + mix(haloColor, vec4(0.), pow(smoothstep(0., 1., dist * .93), .01)) * 10.;
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
