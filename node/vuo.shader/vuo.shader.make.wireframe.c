/**
 * @file
 * vuo.shader.make.wireframe node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Wireframe Shader",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						 "outline", "edges", "lines", "skeletal",
						 "worldspace", "perspective", "screenspace",
					 ],
					 "version" : "1.1.0",
					 "dependencies" : [
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

	void main()
	{
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;

	void main()
	{
		gl_Position = gl_PositionIn[0];
		geometryDistanceFromEdge = vec3(1,0,0);
		EmitVertex();

		gl_Position = gl_PositionIn[1];
		geometryDistanceFromEdge = vec3(0,1,0);
		EmitVertex();

		gl_Position = gl_PositionIn[2];
		geometryDistanceFromEdge = vec3(0,0,1);
		EmitVertex();

		EndPrimitive();
	}
);

static const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from VuoSceneRenderer
	uniform float aspectRatio;
	uniform float primitiveHalfSize;

	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;

	void main()
	{
		vec2 pointSize = vec2(primitiveHalfSize, primitiveHalfSize * aspectRatio);

		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(1,0,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,1,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,0,1);
		EmitVertex();
		EndPrimitive();

		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x, -pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(1,0,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4( pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,1,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + vec4(-pointSize.x,  pointSize.y, 0, 0);
		geometryDistanceFromEdge  = vec3(0,0,1);
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

	// Outputs to fragment shader
	varying out vec3 geometryDistanceFromEdge;

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
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       - perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,1,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,0,1);
		EmitVertex();
		EndPrimitive();

		gl_Position               = gl_PositionIn[0]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(1,0,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[1]       + perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,1,0);
		EmitVertex();
		gl_Position               = gl_PositionIn[1]       - perpendicularOffset;
		geometryDistanceFromEdge  = vec3(0,0,1);
		EmitVertex();
		EndPrimitive();
	}
);

static const char *fragmentShaderSource = VUO_STRINGIFY(
	// Inputs from ports
	uniform vec4 frontLineColor;
	uniform vec4 backLineColor;
	uniform float width;
	uniform float standoffWidth;
	uniform bool showThirdEdge;

	// Inputs from geometry shader
	varying vec3 geometryDistanceFromEdge;

	void main()
	{
		// Work around ATI Radeon HD 5770 bug.
		// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
		// https://b33p.net/kosada/node/11256
		gl_FragColor = frontLineColor;

		vec3 filterWidth = fwidth(geometryDistanceFromEdge) / 2.;

		\n#if SCREENSPACE == 1\n

			// So the width slider has a useful range for both screen-space and model-space.
			float scaledWidth = width * 20.;

			float swidth = (width + standoffWidth) * 20.;

			vec3 smoothEdge         = smoothstep(filterWidth * (scaledWidth - 1.), filterWidth * (scaledWidth + 1.), geometryDistanceFromEdge);
			vec3 smoothStandoffEdge = smoothstep(filterWidth * (swidth      - 1.), filterWidth * (swidth      + 1.), geometryDistanceFromEdge);

			float minimumDistanceFromStandoffEdge = min(smoothStandoffEdge.x, smoothStandoffEdge.z);
			if (showThirdEdge)
				minimumDistanceFromStandoffEdge = min(minimumDistanceFromStandoffEdge, smoothStandoffEdge.y);
			if (minimumDistanceFromStandoffEdge > .999)
				discard;

			float minimumDistanceFromSmoothEdge = min(smoothEdge.x, smoothEdge.z);
			if (showThirdEdge)
				minimumDistanceFromSmoothEdge = min(minimumDistanceFromSmoothEdge, smoothEdge.y);

		\n#else\n // worldspace

			float swidth = width+standoffWidth;
			vec3 smoothEdge         = smoothstep( width - filterWidth,  width + filterWidth, geometryDistanceFromEdge);
			vec3 smoothStandoffEdge = smoothstep(swidth - filterWidth, swidth + filterWidth, geometryDistanceFromEdge);

			float minimumDistanceFromStandoffEdge = min(smoothStandoffEdge.x, smoothStandoffEdge.z);
			if (showThirdEdge)
				minimumDistanceFromStandoffEdge = min(minimumDistanceFromStandoffEdge, smoothStandoffEdge.y);
			if (minimumDistanceFromStandoffEdge > .999)
				discard;

			float minimumDistanceFromSmoothEdge = min(smoothEdge.x, smoothEdge.z);
			if (showThirdEdge)
				minimumDistanceFromSmoothEdge = min(minimumDistanceFromSmoothEdge, smoothEdge.y);

		\n#endif\n

		// Branch to avoid antialiasing junk when either width or standoffWidth is entirely hidden.
		vec4 color = gl_FrontFacing ? frontLineColor : backLineColor;
		if (standoffWidth > 0.)
		{
			if (width > 0.)
				gl_FragColor = mix(
								   mix(color, vec4(0.,0.,0.,1.), minimumDistanceFromSmoothEdge),
								   vec4(0.),
								   minimumDistanceFromStandoffEdge);
			else
				gl_FragColor = mix(vec4(0.,0.,0.,1.), vec4(0.), minimumDistanceFromStandoffEdge);
		}
		else
		{
			if (width > 0.)
				gl_FragColor = vec4(color.rgb, mix(color.a, 0., minimumDistanceFromSmoothEdge));
			else
				discard;
		}
	}
);

void nodeEvent
(
		VuoInputData(VuoColor, {"default":{"r":0.6,"g":0.8,"b":0.6,"a":1}}) frontColor,
		VuoInputData(VuoColor, {"default":{"r":0.3,"g":0.4,"b":0.3,"a":1}}) backColor,
		VuoInputData(VuoReal, {"default":0.2, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.05}) width,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.05}) standoffWidth,
		VuoInputData(VuoBoolean, {"default":true}) showThirdEdge,
		VuoInputData(VuoBoolean, {"default":false}) uniformWidth,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("Wireframe Shader");
	(*shader)->useAlphaAsCoverage = true;

	char *fragmentShaderSourceWithPrefix = VuoText_format("#version 120\n#define SCREENSPACE %d\n\n%s", uniformWidth ? 1 : 0, fragmentShaderSource);

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSource, pointGeometryShaderSource, fragmentShaderSourceWithPrefix);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSource, lineGeometryShaderSource,  fragmentShaderSourceWithPrefix);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSource, geometryShaderSource,      fragmentShaderSourceWithPrefix);

	free(fragmentShaderSourceWithPrefix);

	VuoShader_setUniform_VuoColor(*shader, "frontLineColor", frontColor);
	VuoShader_setUniform_VuoColor(*shader, "backLineColor", backColor);
	VuoShader_setUniform_VuoReal(*shader, "width", width/2.);
	VuoShader_setUniform_VuoReal(*shader, "standoffWidth", standoffWidth/2.);
	VuoShader_setUniform_VuoBoolean(*shader, "showThirdEdge", showThirdEdge);
}
