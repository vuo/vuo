/**
 * @file
 * VuoShader shader definitions.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

/**
 * Projects `position`, and provides an unprojected `position`, used for triangulation.
 */
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
		geometryNormal = (cameraMatrixInverse * modelviewMatrix * vec4(normal, 0.)).xyz;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));
	}
);

/**
 * Helper for @ref VuoShader_makeDefaultShader.
 */
static VuoShader VuoShader_makeDefaultShaderInternal(void)
{
	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		varying vec2 fragmentTextureCoordinate;
		uniform vec4 blah;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = blah;

			// Based on the Gritz/Baldwin antialiased checkerboard shader.

			vec3 color0 = vec3(1   -fragmentTextureCoordinate.x, fragmentTextureCoordinate.y,      1   ) * (gl_FrontFacing ? 1 : .25);
			vec3 color1 = vec3(0.75-fragmentTextureCoordinate.x, fragmentTextureCoordinate.y-0.25, 0.75) * (gl_FrontFacing ? 1 : .25);
			float frequency = 8;
			vec2 filterWidth = fwidth(fragmentTextureCoordinate) * frequency;

			vec2 checkPos = fract(fragmentTextureCoordinate * frequency);
			vec2 p = smoothstep(vec2(0.5), filterWidth + vec2(0.5), checkPos) +
				(1 - smoothstep(vec2(0),   filterWidth,             checkPos));

			gl_FragColor = vec4(mix(color0, color1, p.x*p.y + (1-p.x)*(1-p.y)), 1);
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		varying vec3 fragmentPosition;
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;
		uniform vec4 blah;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = blah;

			fragmentPosition;
			fragmentNormal;
			fragmentTextureCoordinate;
			fragmentVertexColor;

			// Based on the Gritz/Baldwin antialiased checkerboard shader.

			vec3 color0 = vec3(1   -fragmentTextureCoordinate.x, fragmentTextureCoordinate.y,      1);
			vec3 color1 = vec3(0.75-fragmentTextureCoordinate.x, fragmentTextureCoordinate.y-0.25, 0.75);
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

	VuoShader shader = VuoShader_make("Default Shader (Checkerboard)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, NULL,                                       NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "blah", VuoColor_makeWithRGBA(42,42,42,42));

	return shader;
}

/**
 * Returns a shared instance of the default (unlit checkerboard) shader.
 * It's a gradient checkerboard (white in the top-left corner),
 * so you can see the object and get a feel for its texture coordinates.
 *
 * @threadAny
 */
VuoShader VuoShader_makeDefaultShader(void)
{
	static dispatch_once_t once = 0;
	static VuoShader defaultShader;
	dispatch_once(&once, ^{
					  defaultShader = VuoShader_makeDefaultShaderInternal();
					  VuoRegisterSingleton(defaultShader);
					  VuoRegisterSingleton(defaultShader->name);
					  VuoRegisterSingleton(defaultShader->pointProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->pointProgram.geometrySource);
					  VuoRegisterSingleton(defaultShader->pointProgram.fragmentSource);
					  VuoRegisterSingleton(defaultShader->lineProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->lineProgram.geometrySource);
					  VuoRegisterSingleton(defaultShader->lineProgram.fragmentSource);
					  VuoRegisterSingleton(defaultShader->triangleProgram.vertexSource);
					  VuoRegisterSingleton(defaultShader->triangleProgram.fragmentSource);
					  VuoRegisterSingleton(defaultShader->uniforms[0].name);
					  VuoRegisterSingleton(defaultShader->uniforms[0].type);
				  });
	return defaultShader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * @c image must be @c GL_TEXTURE_2D.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitImageShader(VuoImage image, VuoReal alpha)
{
	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform vec4 blah;

		// Inputs from vertex/geometry shader
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = blah;

			vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate) * fragmentVertexColor;
			color *= alpha;
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform vec4 blah;

		// Inputs from vertex/geometry shader
		varying vec3 fragmentPosition;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = blah;

			fragmentPosition;

			vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate) * fragmentVertexColor;
			color *= alpha;
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Unlit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, NULL,                                       NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture", image);
	VuoShader_setUniform_VuoReal (shader, "alpha",   alpha);
	VuoShader_setUniform_VuoColor(shader, "blah",    VuoColor_makeWithRGBA(42,42,42,42));

	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * This shader now handles alpha the same way @ref VuoShader_makeUnlitImageShader does,
 * so the only difference is that this shader also provides flipping.
 *
 * @c image must be @c GL_TEXTURE_2D.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitAlphaPassthruImageShader(VuoImage image, bool flipped)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs from ports
		uniform sampler2D texture;

		// Inputs from vertex/geometry shader
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate);
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	const char *fragmentShaderSourceFlipped = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs from ports
		uniform sampler2D texture;

		// Inputs from vertex/geometry shader
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = VuoGlsl_sample(texture, vec2(fragmentTextureCoordinate.x, 1. - fragmentTextureCoordinate.y));
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Unlit, AlphaPassthru)");
	if (flipped)
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSourceFlipped);
	else
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage(shader, "texture", image);
	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * @c image must be @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
VuoShader VuoShader_makeGlTextureRectangleShader(VuoImage image, VuoReal alpha)
{
	if (!image)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs
		uniform sampler2DRect texture;
		uniform vec2 textureSize;
		uniform float alpha;
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = VuoGlsl_sampleRect(texture, fragmentTextureCoordinate*textureSize);
			color *= alpha;
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (GL_TEXTURE_RECTANGLE)");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "texture",     image);
	VuoShader_setUniform_VuoReal   (shader, "alpha",       alpha);
	VuoShader_setUniform_VuoPoint2d(shader, "textureSize", VuoPoint2d_make(image->pixelsWide, image->pixelsHigh));
	return shader;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting).
 *
 * This shader now handles alpha the same way @ref VuoShader_makeUnlitImageShader does,
 * so the only difference is that this shader also provides flipping.
 *
 * @c image must be @c GL_TEXTURE_RECTANGLE_ARB.
 *
 * @threadAny
 */
VuoShader VuoShader_makeGlTextureRectangleAlphaPassthruShader(VuoImage image, bool flipped)
{
	if (!image)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs
		uniform sampler2DRect texture;
		uniform vec2 textureSize;
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = VuoGlsl_sampleRect(texture, fragmentTextureCoordinate*textureSize);
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	const char *fragmentShaderSourceFlipped = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"

		// Inputs
		uniform sampler2DRect texture;
		uniform vec2 textureSize;
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			vec4 color = VuoGlsl_sampleRect(texture, vec2(fragmentTextureCoordinate.x, 1. - fragmentTextureCoordinate.y) * textureSize);
			VuoGlsl_discardInvisible(color.a);
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (GL_TEXTURE_RECTANGLE, AlphaPassthru)");
	if (flipped)
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSourceFlipped);
	else
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoImage  (shader, "texture",     image);
	VuoShader_setUniform_VuoPoint2d(shader, "textureSize", VuoPoint2d_make(image->pixelsWide, image->pixelsHigh));
	return shader;
}

/**
 * Returns a shader that renders a solid @c color.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitColorShader(VuoColor color)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslProjection.glsl"

		// Inputs from VuoSceneRenderer
		uniform mat4 modelviewMatrix;
		attribute vec3 position;
		attribute vec4 vertexColor;
		uniform bool hasVertexColors;

		// Outputs to fragment shader
		varying vec3 fragmentPosition;
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));
			fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		}
	);

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform vec4 color;

		// Inputs from vertex/geometry shader
		varying vec4 fragmentVertexColor;

		void main()
		{
			gl_FragColor = color * fragmentVertexColor;
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		// Inputs from ports
		uniform vec4 color;

		// Inputs from vertex/geometry shader
		varying vec3 fragmentPosition;
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			fragmentPosition;
			fragmentNormal;
			fragmentTextureCoordinate;

			gl_FragColor = color * fragmentVertexColor;
		}
	);

	VuoShader shader = VuoShader_make("Color Shader (Unlit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, vertexShaderSource,                         NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "color", color);

	return shader;
}

/**
 * Returns a shader that renders a solid @c color circle.
 *
 * When sharpness = 1, the circle takes up half the size of the texture coordinates
 * (circumscribing a rectangle from (0.25,0.25) to (0.75,0.75)).
 *
 * When sharpness = 0, the circle's edge is blurred to take up the entire texture coordinate area.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitCircleShader(VuoColor color, VuoReal sharpness)
{
	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 color;
		uniform float sharpness;
		varying vec2 fragmentTextureCoordinate;

		void main(void)
		{
			float dist = distance(fragmentTextureCoordinate, vec2(0.5,0.5));
			float delta = fwidth(dist);
			gl_FragColor = mix(color, vec4(0.), smoothstep(sharpness/2 - delta, 1 - sharpness/2 + delta, dist*2));
		}
	);

	VuoShader shader = VuoShader_make("Circle Shader");
	shader->objectScale = 0.5;

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, NULL,                                       NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "color", color);
	VuoShader_setUniform_VuoReal (shader, "sharpness", VuoReal_clamp(sharpness, 0, 1));
	return shader;
}

/**
 * Returns a shader that renders a solid @c color checkmark with outline and outline thickness.
 *
 * @threadAny
 * @version200New
 */
VuoShader VuoShader_makeUnlitCheckmarkShader(VuoColor color, VuoColor outline, float thickness)
{
	// The VUOSHADER_GLSL_SOURCE macro gets hung up on the const vec2 (); declaration.
	const char *fragmentShaderSource = "#version 120 \n\
varying vec2 fragmentTextureCoordinate;\
uniform vec4 color;\
uniform vec4 lineColor;\
uniform float thickness;\
const int CHECK_VERTICES = 6;\
const vec2 CHECK[CHECK_VERTICES] = vec2[] (\
	vec2(.0, .47),\
	vec2(.175, .66),\
	vec2(.37, .46),\
	vec2(.82, .9),\
	vec2(1, .72),\
	vec2(.37, .1)\
);\
\
bool doLineSegmentsIntersect(vec2 p0, vec2 p1, vec2 p2, vec2 p3)\
{\
	vec2 s1 = p1 - p0;\
	vec2 s2 = p3 - p2;\
	float s, t;\
	s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / (-s2.x * s1.y + s1.x * s2.y);\
	t = ( s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / (-s2.x * s1.y + s1.x * s2.y);\
\
	return (s >= 0 && s <= 1 && t >= 0 && t <= 1);\
}\
\
float drawLine(vec2 p1, vec2 p2)\
{\
	vec2 uv = fragmentTextureCoordinate;\
	float a = abs(distance(p1, uv));\
	float b = abs(distance(p2, uv));\
	float c = abs(distance(p1, p2));\
	if( a >= c || b >= c) return 0.;\
	float p = (a + b + c) * .5;\
	float h = 2. / c * sqrt(p * (p-a) * (p-b) * (p-c));\
	return mix(1, 0, smoothstep(0.5 * thickness, 1.5 * thickness, h));\
}\
\
bool isPointInPolygon(vec2 point)\
{\
	vec2 rayStart = vec2(0, .5);\
	bool inPoly = false;\
\
	for(int i = 0; i < CHECK_VERTICES; i += 1)\
	{\
		if(doLineSegmentsIntersect(rayStart, point, CHECK[i], CHECK[i == (CHECK_VERTICES-1) ? 0 : i+1]) )\
			inPoly = !inPoly;\
	}\
\
	return inPoly;\
}\
\
void main(void)\
{\
	float line = 0;\
	for(int i = 0; i < CHECK_VERTICES; i++)\
		line = max(drawLine(CHECK[i], CHECK[i < CHECK_VERTICES-1 ? i+1 : 0]), line);\
	gl_FragColor = mix(isPointInPolygon(fragmentTextureCoordinate) ? color : vec4(0), lineColor, line);\
}\
";

	VuoShader shader = VuoShader_make("Checkmark Shader");
	shader->objectScale = 1;
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoColor(shader, "color", color);
	VuoShader_setUniform_VuoColor(shader, "lineColor", outline);
	VuoShader_setUniform_VuoReal(shader, "thickness", thickness);
	return shader;
}

/**
 * Returns a shader that renders a solid @c color rounded rectangle.
 *
 * When sharpness = 1, the rounded rectangle takes up half the size of the texture coordinates
 * (from (0.25,0.25) to (0.75,0.75)).
 *
 * When sharpness = 0, the rounded rectangle's edge is blurred to take up the entire texture coordinate area.
 *
 * `aspect` specifies the aspect ratio of the rectangle.
 *
 * @threadAny
 */
VuoShader VuoShader_makeUnlitRoundedRectangleShader(VuoColor color, VuoReal sharpness, VuoReal roundness, VuoReal aspect)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 color;
		uniform float sharpness;
		uniform float roundness;
		uniform float aspect;
		varying vec2 fragmentTextureCoordinate;

		void main(void)
		{
			float roundness2 = max(1. - sharpness, roundness);
			roundness2 = min(aspect, roundness2);
			float diameter = roundness2 / 2.;
			diameter = max(diameter, 0.001);
			float radius = diameter / 2.;
			vec2 r = vec2(radius/aspect, radius);

			vec2 cornerCircleCenter = vec2(0.,0.);
			if (fragmentTextureCoordinate.x > 0.75 - r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top right corner
					cornerCircleCenter = vec2(0.75, 0.75) + vec2(-r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + r.y)
					// Bottom right corner
					cornerCircleCenter = vec2(0.75, 0.25) + vec2(-r.x,  r.y);
			}
			else if (fragmentTextureCoordinate.x < 0.25 + r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top left corner
					cornerCircleCenter = vec2(0.25, 0.75) + vec2( r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + radius)
					// Bottom left corner
					cornerCircleCenter = vec2(0.25, 0.25) + vec2( r.x,  r.y);
			}

			float dist = 0.;
			if (cornerCircleCenter.x > 0.)
				dist = distance((fragmentTextureCoordinate - cornerCircleCenter) * vec2(aspect, 1.) + cornerCircleCenter, cornerCircleCenter) / diameter;
			else
			{
				float f = 1. - (1. - sharpness) * (1. - sharpness);
				if (aspect < 1.)
					f = 1./f;
				float n = (fragmentTextureCoordinate.x - 0.5) * f;

				if (fragmentTextureCoordinate.y < 0.5 + n)
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Right edge
						dist = (fragmentTextureCoordinate.x - (0.75 - r.x)) * aspect;
					else
						// Bottom edge
						dist = (0.25 + r.y) - fragmentTextureCoordinate.y;
				}
				else
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Top edge
						dist = fragmentTextureCoordinate.y - (0.75 - r.y);
					else
						// Left edge
						dist = ((0.25 + r.x) - fragmentTextureCoordinate.x) * aspect;
				}

				dist /= diameter;
			}

//			float delta = min(fwidth(fragmentTextureCoordinate.x), fwidth(fragmentTextureCoordinate.y));
			float delta = fwidth(fragmentTextureCoordinate.y);
			delta /= diameter;
			delta /= 2.;

			gl_FragColor = mix(color, vec4(0.), smoothstep(sharpness / 2. - delta, 1. - sharpness / 2. + delta, dist));
		}
	);

	VuoShader shader = VuoShader_make("Rounded Rectangle Shader");
	shader->objectScale = 0.5;
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoColor(shader, "color",     color);
	VuoShader_setUniform_VuoReal (shader, "sharpness", VuoReal_clamp(sharpness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "roundness", VuoReal_clamp(roundness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "aspect",    aspect);
	return shader;
}

/**
 * Returns a shader that renders a solid @c color rounded rectangle with a split color based on value.
 *
 * When sharpness = 1, the rounded rectangle takes up half the size of the texture coordinates
 * (from (0.25,0.25) to (0.75,0.75)).
 *
 * When sharpness = 0, the rounded rectangle's edge is blurred to take up the entire texture coordinate area.
 *
 * `aspect` specifies the aspect ratio of the rectangle.
 *
 * @threadAny
 * @version200New
 */
VuoShader VuoShader_makeUnlitRoundedRectangleTrackShader(
	VuoColor backgroundColor,
	VuoColor activeColor,
	VuoReal sharpness,
	VuoReal roundness,
	VuoReal aspect,
	VuoBoolean isHorizontal,
	VuoReal value)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 backgroundColor;
		uniform vec4 activeColor;
		uniform float sharpness;
		uniform float roundness;
		uniform float aspect;
		uniform float progress;
		uniform bool isHorizontal;
		varying vec2 fragmentTextureCoordinate;

		void main(void)
		{
			float roundness2 = max(1. - sharpness, roundness);
			roundness2 = min(aspect, roundness2);
			float diameter = roundness2 / 2.;
			diameter = max(diameter, 0.001);
			float radius = diameter / 2.;
			vec2 r = vec2(radius/aspect, radius);

			vec2 cornerCircleCenter = vec2(0.,0.);
			if (fragmentTextureCoordinate.x > 0.75 - r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top right corner
					cornerCircleCenter = vec2(0.75, 0.75) + vec2(-r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + r.y)
					// Bottom right corner
					cornerCircleCenter = vec2(0.75, 0.25) + vec2(-r.x,  r.y);
			}
			else if (fragmentTextureCoordinate.x < 0.25 + r.x)
			{
				if (fragmentTextureCoordinate.y > 0.75 - r.y)
					// Top left corner
					cornerCircleCenter = vec2(0.25, 0.75) + vec2( r.x, -r.y);
				else if (fragmentTextureCoordinate.y < 0.25 + radius)
					// Bottom left corner
					cornerCircleCenter = vec2(0.25, 0.25) + vec2( r.x,  r.y);
			}

			float dist = 0.;
			if (cornerCircleCenter.x > 0.)
				dist = distance((fragmentTextureCoordinate - cornerCircleCenter) * vec2(aspect, 1.) + cornerCircleCenter, cornerCircleCenter) / diameter;
			else
			{
				float f = 1. - (1. - sharpness) * (1. - sharpness);
				if (aspect < 1.)
					f = 1./f;
				float n = (fragmentTextureCoordinate.x - 0.5) * f;

				if (fragmentTextureCoordinate.y < 0.5 + n)
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Right edge
						dist = (fragmentTextureCoordinate.x - (0.75 - r.x)) * aspect;
					else
						// Bottom edge
						dist = (0.25 + r.y) - fragmentTextureCoordinate.y;
				}
				else
				{
					if (fragmentTextureCoordinate.y > 0.5 - n)
						// Top edge
						dist = fragmentTextureCoordinate.y - (0.75 - r.y);
					else
						// Left edge
						dist = ((0.25 + r.x) - fragmentTextureCoordinate.x) * aspect;
				}

				dist /= diameter;
			}

			float delta = fwidth(dist) / 2.;

			// fwidth() seems to sometimes output invalid results; this hides most of it.
			if (delta > 0.1)
				delta = 0.;

			float val = (isHorizontal ? fragmentTextureCoordinate.x : fragmentTextureCoordinate.y);
			val = (clamp(val, .25, .75) - .25) / .5;
			vec4 color = val < progress ? activeColor : backgroundColor;
			gl_FragColor = mix(color, vec4(0.), smoothstep(sharpness / 2. - delta, 1. - sharpness / 2. + delta, dist));
		}
	);

	VuoShader shader = VuoShader_make("Rounded Rectangle Track Shader");
	shader->objectScale = 0.5;
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoShader_setUniform_VuoColor(shader, "backgroundColor", backgroundColor);
	VuoShader_setUniform_VuoColor(shader, "activeColor", activeColor);
	VuoShader_setUniform_VuoReal (shader, "sharpness", VuoReal_clamp(sharpness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "roundness", VuoReal_clamp(roundness, 0, 1));
	VuoShader_setUniform_VuoReal (shader, "aspect", aspect);
	VuoShader_setUniform_VuoBoolean (shader, "isHorizontal", isHorizontal);
	VuoShader_setUniform_VuoReal (shader, "progress", value);
	return shader;
}

/**
 * Returns a shader that renders a color with lighting.
 *
 * @param diffuseColor The primary material color.
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitColorShader(VuoColor diffuseColor, VuoColor highlightColor, VuoReal shininess)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslProjection.glsl"

		// Inputs provided by VuoSceneRenderer
		uniform mat4 modelviewMatrix;
		attribute vec3 position;
		attribute vec3 normal;
		attribute vec4 vertexColor;
		uniform bool hasVertexColors;

		// Outputs to fragment shader
		varying vec3 fragmentPosition;
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			fragmentTextureCoordinate;

			fragmentPosition = (modelviewMatrix * vec4(position, 1.)).xyz;
			fragmentNormal = (modelviewMatrix *  vec4(normal, 0.)).xyz;
			fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);
			gl_Position = VuoGlsl_projectPosition(fragmentPosition);
		}
	);

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "lighting.glsl"

		// Inputs from ports
		uniform vec4 diffuseColor;
		uniform vec4 specularColor;
		uniform float specularPower;

		// Inputs from vertex/geometry shader
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = diffuseColor;

			fragmentTextureCoordinate;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, fragmentNormal, ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= diffuseColor.rgb * fragmentVertexColor.rgb;
			diffuseContribution *= diffuseColor.rgb * fragmentVertexColor.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, diffuseColor.a * fragmentVertexColor.a);
		}
	);

	VuoShader shader = VuoShader_make("Color Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              defaultVertexShaderSourceForGeometryShader, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     defaultVertexShaderSourceForGeometryShader, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, vertexShaderSource,                         NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoColor(shader, "diffuseColor", diffuseColor);
	VuoShader_setUniform_VuoColor(shader, "specularColor", highlightColor);
	VuoShader_setUniform_VuoReal (shader, "specularPower", 1./(1.0001-shininess));
	return shader;
}

/**
 * A linear-projection vertex shader.
 */
static const char *lightingVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslProjection.glsl"

	// Inputs provided by VuoSceneRenderer
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
		fragmentNormal = normalize(vec3(modelviewMatrix * vec4(normal, 0.)));
		fragmentTextureCoordinate = textureCoordinate;
		fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(fragmentPosition);
	}
);

/**
 * A linear-projection vertex shader.
 */
static const char *lightingVertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
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
		geometryNormal = normalize((modelviewMatrix * vec4(normal, 0.)).xyz);
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = hasVertexColors ? vertexColor : vec4(1.);
		gl_Position = VuoGlsl_projectPosition(geometryPosition);
	}
);

/**
 * Returns a shader that renders an image with lighting.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitImageShader(VuoImage image, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return NULL;

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "lighting.glsl"

		// Inputs from vertex shader
		varying vec3 fragmentNormal;
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform vec4 specularColor;
		uniform float specularPower;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = specularColor;

			vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate) * fragmentVertexColor;
			color *= alpha;
			VuoGlsl_discardInvisible(color.a);

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, fragmentNormal, ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb;
			diffuseContribution *= color.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a);
		}
	);

	VuoShader shader = VuoShader_make("Image Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              lightingVertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     lightingVertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, lightingVertexShaderSource,			  NULL,                      fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture",       image);
	VuoShader_setUniform_VuoReal (shader, "alpha",         alpha);
	VuoShader_setUniform_VuoColor(shader, "specularColor", highlightColor);
	VuoShader_setUniform_VuoReal (shader, "specularPower", 1./(1.0001-shininess));

	return shader;
}

/**
 * Returns a shader that renders an image with lighting and surface details.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param specularImage An image that specifies the specular color (RGB) and shininess (A).
 * @param normalImage An image that specifies the surface details.  The red and green channels respectively define the normal direction along the tangent and bitangent axes (0 = negative; 0.5 = straight; 1 = positive).  The blue channel defines the height along the normal axis (0 = low; 1 = high).
 *
 * @threadAny
 */
VuoShader VuoShader_makeLitImageDetailsShader(VuoImage image, VuoReal alpha, VuoImage specularImage, VuoImage normalImage)
{
	if (!image || !specularImage || !normalImage)
		return NULL;

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	static const char *triangleGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslTangent.glsl"

		uniform mat4 modelviewMatrix;

		// Inputs
		varying in vec3 geometryPosition[3];
		varying in vec3 geometryNormal[3];
		varying in vec2 geometryTextureCoordinate[3];
		varying in vec4 geometryVertexColor[3];

		// Outputs
		varying out vec3 fragmentPosition;
		varying out vec2 fragmentTextureCoordinate;
		varying out vec4 fragmentVertexColor;
		varying out mat3 vertexPlaneToWorld;

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
				fragmentTextureCoordinate = geometryTextureCoordinate[i];
				fragmentVertexColor = geometryVertexColor[i];

				vertexPlaneToWorld[0] = normalize(vec3(modelviewMatrix *  vec4(to.tangent[i],   0.)));
				vertexPlaneToWorld[1] = normalize(vec3(modelviewMatrix * -vec4(to.bitangent[i], 0.)));
				vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix *  vec4(geometryNormal[i],    0.)));

				EmitVertex();
			}
			EndPrimitive();
		}
	);

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "lighting.glsl"

		// Inputs from vertex shader
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;
		varying mat3 vertexPlaneToWorld;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform sampler2D specularImage;
		uniform sampler2D normalImage;
		uniform vec4 blah;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = blah;

			vec4 color = VuoGlsl_sample(texture, fragmentTextureCoordinate);
			color *= alpha;
			VuoGlsl_discardInvisible(color.a);

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			vec4 specularColor = texture2D(specularImage, fragmentTextureCoordinate);
			float specularPower = 1./(1.0001-specularColor.a);

			vec4 normalColor = texture2D(normalImage, fragmentTextureCoordinate);
			vec3 normal = normalize(vec3(
						2. * normalColor.r - 1.,
						2. * normalColor.g - 1.,
						normalColor.b	// Leave the blue channel as-is; the normal should never point inward.
						));

			vec3 normalDirection = vertexPlaneToWorld * normal;

			calculateLighting(specularPower, normalDirection, ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb * fragmentVertexColor.rgb;
			diffuseContribution *= color.rgb * fragmentVertexColor.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a * fragmentVertexColor.a);
		}
	);

	VuoShader shader = VuoShader_make("Image Details Shader (Lit)");

	VuoShader_addSource                      (shader, VuoMesh_Points,              lightingVertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualLines,     lightingVertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSource);
	VuoShader_setExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (shader, VuoMesh_IndividualTriangles, lightingVertexShaderSourceForGeometry, triangleGeometryShaderSource, fragmentShaderSource);

	VuoShader_setUniform_VuoImage(shader, "texture",       image);
	VuoShader_setUniform_VuoReal (shader, "alpha",         alpha);
	VuoShader_setUniform_VuoImage(shader, "specularImage", specularImage);
	VuoShader_setUniform_VuoImage(shader, "normalImage",   normalImage);
	VuoShader_setUniform_VuoColor(shader, "blah",          VuoColor_makeWithRGBA(42,42,42,42));

	return shader;
}

/**
 * Returns a linear gradient shader.
 *
 * @threadAny
 */
VuoShader VuoShader_makeLinearGradientShader(void)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "VuoGlslRandom.glsl"

		uniform float inputColorCount;
		uniform float stripColorCount;
		uniform sampler2D gradientStrip;
		uniform vec2 start;
		uniform vec2 end;
		uniform float aspect;
		uniform float noiseAmount;

		varying vec2 fragmentTextureCoordinate;

		float distSqr(vec2 a, vec2 b)
		{
			return (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
		}

		// https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
		// ...minus the segment part, of course.
		vec2 nearest_point_on_line(vec2 v, vec2 w, vec2 p)
		{
			// Return minimum distance between line segment vw and point p
			float l2 = distSqr(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
			if (l2 == 0.0) return p;
			// Consider the line extending the segment, parameterized as v + t (w - v).
			// We find projection of point p onto the line.
			// It falls where t = [(p-v) . (w-v)] / |w-v|^2
			float t = dot(p - v, w - v) / l2;
			vec2 projection = v + t * (w - v);
			return projection;
		}

		void main(void)
		{
			vec2 tcAspect = fragmentTextureCoordinate;
			tcAspect.y -= .5;
			tcAspect.y *= aspect;
			tcAspect.y += .5;

			vec2 pol = nearest_point_on_line(start, end, tcAspect);
			float x = dot(pol-start, end-start) > 0 ? distance(start, pol)/ distance(start, end) : 0;

			// Give x a smooth second-derivative, to reduce the ridges between colors.
			x *= inputColorCount - 1.;
			x = floor(x) + smoothstep(0.,1.,fract(x));
			x /= inputColorCount - 1.;

			float gradientWidth = (1./stripColorCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;	// scale to account for the gradient/2 offsets

			vec4 color = VuoGlsl_sample(gradientStrip, vec2(clamp(x , gradientWidth, 1.-gradientWidth), .5));
			VuoGlsl_discardInvisible(color.a);

			color.rgb += (VuoGlsl_random2D3D(fragmentTextureCoordinate) - 0.5) * noiseAmount;

			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Linear Gradient Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	return shader;
}

/**
 * Creates a gradient strip texture, and sets shader uniforms.
 */
static void VuoShader_setGradientStrip(VuoShader shader, VuoList_VuoColor colors)
{
	// https://b33p.net/kosada/node/12582
	// Instead of creating an image with only one pixel per color stop,
	// create `gradientExpansion` pixels per stop, to compensate for GPUs
	// that have limited ability to interpolate between pixels.
	// E.g., the Intel HD Graphics 3000 GPU limits interpolation to 64 steps.
	// On Intel 3000, gradientExpansion=4 would result in 256 steps (64*4);
	// go beyond that to provide some extra detail when rendering to 16bpc textures.
	int gradientExpansion = 16;

	GLenum format = GL_BGRA;
	int bpp = 4;
	if (VuoColor_areAllOpaque(colors))
	{
		format = GL_BGR;
		bpp = 3;
	}

	int inputColorCount = VuoListGetCount_VuoColor(colors);
	int stripColorCount = (inputColorCount - 1) * gradientExpansion + 1;

	unsigned char *pixels = (unsigned char*)malloc(stripColorCount * bpp);
	int inputColor = 1;
	int step = 0;
	for (int i = 0; i < stripColorCount; ++i)
	{
		VuoColor col1 = VuoListGetValue_VuoColor(colors, inputColor);
		VuoColor col2 = VuoListGetValue_VuoColor(colors, inputColor+1);

		VuoColor col = VuoColor_lerp(col1, col2, (float)step / gradientExpansion);

		pixels[i * bpp        ] = VuoInteger_clamp(col.a * col.b * 255, 0, 255);
		pixels[i * bpp + 1    ] = VuoInteger_clamp(col.a * col.g * 255, 0, 255);
		pixels[i * bpp + 2    ] = VuoInteger_clamp(col.a * col.r * 255, 0, 255);
		if (bpp == 4)
			pixels[i * bpp + 3] = VuoInteger_clamp(col.a         * 255, 0, 255);

		++step;
		if (step >= gradientExpansion)
		{
			step = 0;
			++inputColor;
		}
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, format, stripColorCount, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	VuoShader_setUniform_VuoImage  (shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal   (shader, "inputColorCount", inputColorCount);
	VuoShader_setUniform_VuoReal   (shader, "stripColorCount", stripColorCount);
}

/**
 * Sets parameters for the linear gradient shader using the provided colors and start and end coordinates.
 * Coordinates should be passed in Vuo scene coordinates (-1,-1) to (1,1).
 */
void VuoShader_setLinearGradientShaderValues(VuoShader shader, VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end, VuoReal aspect, VuoReal noiseAmount)
{
	VuoShader_setGradientStrip(shader, colors);
	VuoShader_setUniform_VuoPoint2d(shader, "start", VuoPoint2d_make((start.x+1)/2, (start.y+1)/2));
	VuoShader_setUniform_VuoPoint2d(shader, "end", VuoPoint2d_make((end.x+1)/2, (end.y+1)/2));
	VuoShader_setUniform_VuoReal   (shader, "aspect", 1./aspect);
	VuoShader_setUniform_VuoReal   (shader, "noiseAmount", MAX(0.,noiseAmount/10.));
}

/**
 * Returns a radial gradient shader.
 *
 * @threadAny
 */
VuoShader VuoShader_makeRadialGradientShader(void)
{
	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "VuoGlslRandom.glsl"

		uniform float inputColorCount;
		uniform float stripColorCount;
		uniform sampler2D gradientStrip;
		uniform vec2 center;
		uniform vec2 scale;	// if image is not square, multiply texCoord by this to account for stretch
		uniform float radius;
		uniform float noiseAmount;

		varying vec2 fragmentTextureCoordinate;

		void main(void)
		{
			vec2 scaledTexCoord = fragmentTextureCoordinate*scale;
			float x = distance(center*scale, scaledTexCoord)/radius;

			// Give x a smooth second-derivative, to reduce the ridges between colors.
			x *= inputColorCount - 1.;
			x = floor(x) + smoothstep(0.,1.,fract(x));
			x /= inputColorCount - 1.;

			float gradientWidth = (1./stripColorCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;

			vec4 color = VuoGlsl_sample(gradientStrip, vec2(clamp(x , gradientWidth, 1.-gradientWidth), .5));
			VuoGlsl_discardInvisible(color.a);

			color.rgb += (VuoGlsl_random2D3D(fragmentTextureCoordinate) - 0.5) * noiseAmount;

			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Radial Gradient Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	return shader;
}

/**
 * Sets parameters for the radial gradient shader using the provided colors, center point, and radius.
 * Center and radius are expected in Vuo scene coordinates.  Width and Height may be either pixels or scene coordinates, as they
 * are only used to calculate the aspect ratio.
 */
void VuoShader_setRadialGradientShaderValues(VuoShader shader, VuoList_VuoColor colors, VuoPoint2d center, VuoReal radius, VuoReal width, VuoReal height, VuoReal noiseAmount)
{
	// VuoPoint2d scale = width < height ? VuoPoint2d_make(1., height/(float)width) : VuoPoint2d_make(width/(float)height, 1.);
	VuoPoint2d scale = VuoPoint2d_make(1., height/(float)width);

	VuoShader_setGradientStrip(shader, colors);
	VuoShader_setUniform_VuoPoint2d(shader, "center", VuoPoint2d_make((center.x+1)/2, (center.y+1)/2));
	VuoShader_setUniform_VuoReal   (shader, "radius", radius > 0. ? radius/2. : 0);
	VuoShader_setUniform_VuoPoint2d(shader, "scale",  VuoPoint2d_make(scale.x, scale.y));
	VuoShader_setUniform_VuoReal   (shader, "noiseAmount", MAX(0.,noiseAmount/10.));
}

/**
 * Returns a frosted glass shader.
 */
VuoShader VuoShader_makeFrostedGlassShader(void)
{
	const char *vertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
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

	const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, \n#include "trianglePoint.glsl");
	const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, \n#include "triangleLine.glsl");

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "noise3D.glsl"

		// Inputs provided by VuoSceneRenderer
		uniform sampler2D colorBuffer;
		uniform vec2 viewportSize;

		// Inputs from ports
		uniform vec4 color;
		uniform float aspectRatio;
		uniform vec2 noisePosition;
		uniform float noiseTime;
		uniform float noiseAmount;
		uniform float noiseScale;
		uniform float chromaticAberration;
		uniform int iterations;
		uniform int levels;
		uniform float roughness;
		uniform float spacing;

		// Inputs from vertex shader
		varying vec2 fragmentTextureCoordinate;
		varying vec4 fragmentVertexColor;

		void main()
		{
			vec2 viewportTextureCoordinate = gl_FragCoord.xy/viewportSize;

			vec4 accumulatedColor = vec4(0.);
			for (int i = 0; i < iterations; ++i)
			{
				// 3D noise, since we want a continuous 2D texture that moves continuously through time.
				// The iteration index needn't be continuous.
				vec3 noiseCoordinate = vec3(fragmentTextureCoordinate.x - .5 - noisePosition.x + float(i), (fragmentTextureCoordinate.y - .5 - noisePosition.y) / aspectRatio, noiseTime);
				noiseCoordinate.xy *= noiseScale;
				vec2 noiseOffset = snoise3D2DFractal(noiseCoordinate, levels, roughness, spacing);

				// Red
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount * (1. - chromaticAberration/3.)) * vec4(1.,0.,0.,1./3.);

				// Green
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount)                                * vec4(0.,1.,0.,1./3.);

				// Blue
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount * (1. + chromaticAberration/3.)) * vec4(0.,0.,1.,1./3.);
			}

			vec4 c = accumulatedColor / float(iterations);
			c.rgb /= c.a;
			c *= color * fragmentVertexColor;
			c.rgb = clamp(c.rgb, 0., 1.);
			c.rgb *= c.a;
			gl_FragColor = c;
		}
	);

	const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
		\n#include "VuoGlslAlpha.glsl"
		\n#include "noise3D.glsl"

		// Inputs provided by VuoSceneRenderer
		uniform sampler2D colorBuffer;
		uniform vec2 viewportSize;

		// Inputs from ports
		uniform vec4 color;
		uniform float aspectRatio;
		uniform vec2 noisePosition;
		uniform float noiseTime;
		uniform float noiseAmount;
		uniform float noiseScale;
		uniform float chromaticAberration;
		uniform int iterations;
		uniform int levels;
		uniform float roughness;
		uniform float spacing;

		// Inputs from geometry shader
		varying vec3 fragmentPosition;
		varying vec2 fragmentTextureCoordinate;

		void main()
		{
			// Work around ATI Radeon HD 5770 bug.
			// It seems that the rest of the shader isn't executed unless we initialize the output with a uniform.
			// https://b33p.net/kosada/node/11256
			gl_FragColor = color;

			fragmentPosition;

			vec2 viewportTextureCoordinate = gl_FragCoord.xy/viewportSize;

			vec4 accumulatedColor = vec4(0.);
			for (int i = 0; i < iterations; ++i)
			{
				// 3D noise, since we want a continuous 2D texture that moves continuously through time.
				// The iteration index needn't be continuous.
				vec3 noiseCoordinate = vec3(fragmentTextureCoordinate.x - .5 - noisePosition.x + float(i), (fragmentTextureCoordinate.y - .5 - noisePosition.y) / aspectRatio, noiseTime);
				noiseCoordinate.xy *= noiseScale;
				vec2 noiseOffset = snoise3D2DFractal(noiseCoordinate, levels, roughness, spacing);

				// Red
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount * (1. - chromaticAberration/3.)) * vec4(1.,0.,0.,1./3.);

				// Green
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount)                                * vec4(0.,1.,0.,1./3.);

				// Blue
				accumulatedColor += VuoGlsl_sample(colorBuffer, viewportTextureCoordinate + noiseOffset * noiseAmount * (1. + chromaticAberration/3.)) * vec4(0.,0.,1.,1./3.);
			}

			vec4 c = accumulatedColor / float(iterations);
			c.rgb /= c.a;
			c *= color;
			c.rgb = clamp(c.rgb, 0., 1.);
			c.rgb *= c.a;
			gl_FragColor = c;
		}
	);

	VuoShader s = VuoShader_make("Frosted Glass Shader");
	s->isTransparent = true;

	VuoShader_addSource                      (s, VuoMesh_Points,              vertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(s, VuoMesh_Points, 2);

	VuoShader_addSource                      (s, VuoMesh_IndividualLines,     vertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(s, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (s, VuoMesh_IndividualTriangles, NULL,                          NULL,                      fragmentShaderSource);

	return s;
}

/**
 * Sets parameters for the frosted glass shader.
 *
 * @version200Changed{Added `noisePosition` and `aspectRatio` arguments.}
 */
void VuoShader_setFrostedGlassShaderValues(VuoShader shader, VuoColor color, VuoReal brightness, VuoPoint2d noisePosition, VuoReal noiseTime, VuoReal noiseAmount, VuoReal noiseScale, VuoReal chromaticAberration, VuoInteger levels, VuoReal roughness, VuoReal spacing, VuoInteger iterations, float aspectRatio)
{
	VuoShader_setUniform_VuoPoint4d(shader, "color",               VuoPoint4d_make(color.r*brightness, color.g*brightness, color.b*brightness, color.a));
	VuoShader_setUniform_VuoPoint2d(shader, "noisePosition",       (VuoPoint2d){(noisePosition.x+1)/2,
																				(noisePosition.y+1)/2 * aspectRatio});
	VuoShader_setUniform_VuoReal   (shader, "noiseTime",           noiseTime);
	VuoShader_setUniform_VuoReal   (shader, "noiseAmount",         MAX(0.,noiseAmount/10.));
	VuoShader_setUniform_VuoReal   (shader, "noiseScale",          1./VuoReal_makeNonzero(noiseScale));
	VuoShader_setUniform_VuoReal   (shader, "chromaticAberration", VuoReal_clamp(chromaticAberration, 0, 2));
	VuoShader_setUniform_VuoInteger(shader, "iterations",          MAX(1, iterations));
	VuoShader_setUniform_VuoInteger(shader, "levels",              MAX(1, levels));
	VuoShader_setUniform_VuoReal   (shader, "roughness",           roughness);
	VuoShader_setUniform_VuoReal   (shader, "spacing",             spacing);
}
