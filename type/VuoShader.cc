/**
 * @file
 * VuoShader implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

extern "C"
{
#include "node.h"
#include "type.h"
#include "VuoShader.h"
#include "VuoGlPool.h"

#include <OpenGL/CGLMacro.h>


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Graphics Shader",
					 "description" : "A graphics shader program, specifying how to render a 3D object.",
					 "keywords" : [ "glsl", "fragment", "vertex" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoColor",
						"VuoImage",
						"VuoInteger",
						"VuoMesh",
						"VuoPoint2d",
						"VuoPoint3d",
						"VuoPoint4d",
						"VuoReal",
						"VuoText",
						"VuoList_VuoInteger",
						"VuoList_VuoImage",
						"VuoList_VuoColor",
						"VuoList_VuoText",
						"VuoGlContext",
						"VuoGlPool",
						"OpenGL.framework"
					 ]
				 });
#endif
/// @}
}

/**
 * Frees the CPU memory and GPU objects associated with the shader.
 *
 * @threadAny
 */
void VuoShader_free(void *shader)
{
	VuoShader s = (VuoShader)shader;

	// Don't delete the GL Program or Shader objects, since @ref VuoGlShader_use and @ref VuoGlProgram_use expect them to persist.

	VuoRelease(s->name);

	VuoRelease(s->pointProgram.vertexSource);
	VuoRelease(s->pointProgram.geometrySource);
	VuoRelease(s->pointProgram.fragmentSource);

	VuoRelease(s->lineProgram.vertexSource);
	VuoRelease(s->lineProgram.geometrySource);
	VuoRelease(s->lineProgram.fragmentSource);

	VuoRelease(s->triangleProgram.vertexSource);
	VuoRelease(s->triangleProgram.geometrySource);
	VuoRelease(s->triangleProgram.fragmentSource);

	for (unsigned int u = 0; u < s->uniformsCount; ++u)
	{
		VuoRelease(s->uniforms[u].name);

		if (strcmp(s->uniforms[u].type, "VuoImage") == 0)
			VuoRelease(s->uniforms[u].value.image);

		VuoRelease(s->uniforms[u].type);
	}

	free(s->uniforms);

	dispatch_release((dispatch_semaphore_t)s->lock);

	free(s);
}

/**
 * Creates a shader object, which contains multiple GL Program Objects.
 *
 * Before using this shader, call @ref VuoShader_addSource at least once.
 *
 * @param name Text describing the shader, displayed in port popovers.  Use title case (e.g., `Color Shader (Lit)`, `Crop Image Shader`, `Threshold Shader (Luminance)`).  The value is copied, so you can free it after this function returns.
 *
 * @threadAny
 */
VuoShader VuoShader_make(const char *name)
{
	VuoShader t = (VuoShader)calloc(1, sizeof(struct _VuoShader));
	VuoRegister(t, VuoShader_free);

	t->name = VuoText_make(name);
	VuoRetain(t->name);

	t->pointProgram.expectedOutputPrimitiveCount = 1;
	t->lineProgram.expectedOutputPrimitiveCount = 1;
	t->triangleProgram.expectedOutputPrimitiveCount = 1;

	t->objectScale = 1;

	t->isTransparent = false;

	t->colorBuffer = NULL;
	t->depthBuffer = NULL;

	t->lock = dispatch_semaphore_create(1);

	return t;
}

/**
 * Creates an unlit color shader object.
 */
VuoShader VuoShader_make_VuoColor(VuoColor color)
{
	// Keep in sync with vuo.shader.make.image.
	VuoColor defaultHighlightColor = VuoColor_makeWithRGBA(1,1,1,1);
	VuoReal defaultShininess = 0.9;

	return VuoShader_makeLitColorShader(color, defaultHighlightColor, defaultShininess);
}

/**
 * Returns the passed shader (does not make a copy).
 */
VuoShader VuoShader_make_VuoShader(VuoShader shader)
{
	return shader;
}

/**
 * Creates an unlit image shader.
 */
VuoShader VuoShader_make_VuoImage(VuoImage image)
{
	// Keep in sync with vuo.shader.make.image.
	VuoColor defaultHighlightColor = VuoColor_makeWithRGBA(1,1,1,1);
	VuoReal defaultShininess = 0.9;

	return VuoShader_makeLitImageShader(image, 1, defaultHighlightColor, defaultShininess);
}


/**
 * Defines a `program` variable and initializes it with the relevant program based on `inputPrimitiveMode`.
 */
#define DEFINE_PROGRAM()									\
	VuoSubshader *program;									\
	VuoMesh_ElementAssemblyMethod epm = VuoMesh_getExpandedPrimitiveMode(inputPrimitiveMode); \
	if (epm == VuoMesh_IndividualTriangles)					\
		program = &shader->triangleProgram;					\
	else if (epm == VuoMesh_IndividualLines)				\
		program = &shader->lineProgram;						\
	else /* if (inputPrimitiveMode == VuoMesh_Points) */	\
		program = &shader->pointProgram;

/**
 * Associates GLSL shader source code with the specified `inputPrimitiveMode` of the specified `shader`.
 * (The compile and link steps are deferred until the shader is actually used.)
 *
 * May be called multiple times, to enable this shader to support multiple `inputPrimitiveMode`s.
 *
 * Call before using @ref VuoShader_getAttributeLocations, @ref VuoShader_activate, and @ref VuoShader_deactivate.
 *
 * @param shader The shader to modify.
 * @param inputPrimitiveMode The type of input primitives this shader program will process.  Should be @ref VuoMesh_IndividualTriangles, @ref VuoMesh_IndividualLines, or @ref VuoMesh_Points.
 * @param vertexShaderSource GLSL vertex shader source code.  If `NULL`, a default vertex shader is used (it passes texture coordinates through, and projects the vertex).
 * @param geometryShaderSource GLSL geometry shader source code.  Optional (may be `NULL`), in which case the output of the vertex shader is passed through to the fragment shader.
 * @param fragmentShaderSource GLSL fragment shader source code.  Optional (may be `NULL`), in which case the shader is assumed to transform primitives into other primitives (rather than pixels), i.e., transform feedback.
 *
 * @see VUOSHADER_GLSL_SOURCE
 *
 * @ref VuoSceneRenderer renders a @ref VuoSceneObject into an image or window,
 * and automatically provides several uniform values and vertex attributes to shaders.
 * A few are required; the rest are optional.
 *
 *     uniform mat4 projectionMatrix;  // required
 *     uniform bool useFisheyeProjection;	// When true, projectionMatrix is calculated assuming that the vertex shader will perform fisheye warping.  See `VuoGlslProjection.glsl`.
 *
 *     uniform mat4 modelviewMatrix;   // required
 *
 *     uniform mat4 cameraMatrixInverse;	// Inverse of the matrix's modelviewMatrix.  Apply this to worldspace coordinates to transform the scene such that the camera is at the origin.
 *     uniform vec3 cameraPosition;
 *
 *     uniform float aspectRatio;	// Viewport aspect ratio.
 *     uniform vec2 viewportSize;	// Viewport size in pixels.
 *     uniform sampler2D colorBuffer;	// A color image of what's been rendered so far (just before executing this shader).
 *     uniform sampler2D depthBuffer;	// A depth image of what's been rendered so far (just before executing this shader).
 *
 *     float primitiveSize;
 *
 *     uniform vec4 ambientColor;
 *     uniform float ambientBrightness;
 *
 *     struct PointLight
 *     {
 *         vec4 color;
 *         float brightness;
 *         vec3 position;
 *         float range;
 *         float sharpness;
 *     };
 *     uniform PointLight pointLights[16];
 *     uniform int pointLightCount;
 *
 *     struct DirectionalLight
 *     {
 *         vec4 color;
 *         float brightness;
 *         vec3 position;
 *         vec3 direction;
 *         float cone;
 *         float range;
 *         float sharpness;
 *     };
 *     uniform DirectionalLight directionalLights[16];
 *     uniform int directionalLightCount;
 *
 *     attribute vec4 position;  // required
 *     attribute vec4 normal;
 *     attribute vec4 tangent;
 *     attribute vec4 bitangent;
 *     attribute vec4 textureCoordinate;
 *     uniform bool hasTextureCoordinates;
 *
 * @ref VuoSceneObjectRenderer renders a @ref VuoSceneObject into a @ref VuoSceneObject,
 * and automatically provides several uniform values and vertex attributes to shaders:
 *
 *     uniform mat4 modelviewMatrix;
 *     uniform mat4 modelviewMatrixInverse;
 *     attribute vec4 position;
 *     attribute vec4 normal;
 *     attribute vec4 tangent;
 *     attribute vec4 bitangent;
 *     attribute vec4 textureCoordinate;
 *
 * And it expects as output:
 *
 *     varying vec4 outPosition;
 *     varying vec4 outNormal;
 *     varying vec4 outTangent;
 *     varying vec4 outBitangent;
 *     varying vec4 outTextureCoordinate;
 *
 * @threadAny
 */
void VuoShader_addSource(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const char *vertexShaderSource, const char *geometryShaderSource, const char *fragmentShaderSource)
{
	if (!shader)
		return;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();

	if (vertexShaderSource)
		program->vertexSource = VuoText_make(vertexShaderSource);
	else
	{
		const char *defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
			include(VuoGlslProjection)

			// Inputs
			uniform mat4 modelviewMatrix;
			attribute vec4 position;
			attribute vec4 textureCoordinate;

			// Outputs to fragment shader
			varying vec4 fragmentTextureCoordinate;

			void main()
			{
				fragmentTextureCoordinate = textureCoordinate;

				gl_Position = VuoGlsl_projectPosition(modelviewMatrix * position);
			}
		);
		program->vertexSource = VuoText_make(defaultVertexShaderSource);
	}
	VuoRetain(program->vertexSource);

	if (geometryShaderSource)
	{
		program->geometrySource = VuoText_make(geometryShaderSource);
		VuoRetain(program->geometrySource);
	}

	if (fragmentShaderSource)
	{
		program->fragmentSource = VuoText_make(fragmentShaderSource);
		VuoRetain(program->fragmentSource);
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
}

/**
 * Specifies the number of primitives the geometry shader is expected to produce per invocation.
 * If this function is not called, the shader defaults to expecting 1 primitive.
 *
 * Call before using @ref VuoShader_getAttributeLocations, @ref VuoShader_activate, and @ref VuoShader_deactivate.
 *
 * @param shader The shader to modify.
 * @param inputPrimitiveMode The shader program mode to modify.
 * @param expectedOutputPrimitiveCount The number of primitives the geometry shader is expected to produce per invocation.
 *
 * @threadAny
 */
void VuoShader_setExpectedOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const unsigned int expectedOutputPrimitiveCount)
{
	if (!shader)
		return;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	program->expectedOutputPrimitiveCount = expectedOutputPrimitiveCount;

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
}

/**
 * Specifies whether the geometry shader may dynamically choose to skip outputting some primitives or output additional primitives.
 *
 * If true, performance is potentially reduced since VuoSceneObjectRenderer will need to wait for the result of a query to determine the actual number of output primitives.
 *
 * Call before using @ref VuoShader_getAttributeLocations, @ref VuoShader_activate, and @ref VuoShader_deactivate.
 *
 * @param shader The shader to modify.
 * @param inputPrimitiveMode The shader program mode to modify.
 * @param mayChangeOutputPrimitiveCount Whether the geometry shader may dynamically choose to skip outputting some primitives or output additional primitives.
 *
 * @threadAny
 */
void VuoShader_setMayChangeOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const bool mayChangeOutputPrimitiveCount)
{
	if (!shader)
		return;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	program->mayChangeOutputPrimitiveCount = mayChangeOutputPrimitiveCount;

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
}

/**
 * Returns the number of primitives the geometry shader is expected to produce per invocation.
 *
 * @param shader The shader to query.
 * @param inputPrimitiveMode The shader program mode to query.
 *
 * @threadAny
 */
unsigned int VuoShader_getExpectedOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode)
{
	if (!shader)
		return 0;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	unsigned int expectedOutputPrimitiveCount = program->expectedOutputPrimitiveCount;

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);

	return expectedOutputPrimitiveCount;
}

/**
 * Returns true if the geometry shader may not output the expected primitive count.
 *
 * @param shader The shader to query.
 * @param inputPrimitiveMode The shader program mode to query.
 *
 * @threadAny
 */
bool VuoShader_getMayChangeOutputPrimitiveCount(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode)
{
	if (!shader)
		return 0;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	unsigned int mayChangeOutputPrimitiveCount = program->mayChangeOutputPrimitiveCount;

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);

	return mayChangeOutputPrimitiveCount;
}

/**
 * Returns `true` if each of `shader`'s defined `inputPrimitiveMode`s has a vertex shader (and optionally a geometry shader) but lacks a fragment shader.
 *
 * @threadAny
 */
bool VuoShader_isTransformFeedback(VuoShader shader)
{
	if (!shader)
		return false;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	if (shader->pointProgram.glFragmentShaderName
	 || shader->lineProgram.glFragmentShaderName
	 || shader->triangleProgram.glFragmentShaderName)
	{
		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return false;
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return true;
}

/**
 * Ensures that the source code for the specified `inputPrimitiveMode` is compiled, linked, and uploaded.
 * If the shader is NULL, or there is no source code for the specified `inputPrimitiveMode`, or it fails to compile or link, returns `false`.
 *
 * Must be called while `shader->lock` is locked.
 */
static bool VuoShader_ensureUploaded(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext)
{
	if (!shader)
		return false;

	DEFINE_PROGRAM();

	// Is the shader already compiled/linked/uploaded?
	if (program->program.programName)
		return true;

	// Is there source code available?
	if (!program->vertexSource)
		return false;

	program->glVertexShaderName = VuoGlShader_use(glContext, GL_VERTEX_SHADER, program->vertexSource);
	if (program->geometrySource)
		program->glGeometryShaderName = VuoGlShader_use(glContext, GL_GEOMETRY_SHADER_EXT, program->geometrySource);
	if (program->fragmentSource)
		program->glFragmentShaderName = VuoGlShader_use(glContext, GL_FRAGMENT_SHADER, program->fragmentSource);

	program->program = VuoGlProgram_use(glContext, shader->name, program->glVertexShaderName, program->glGeometryShaderName, program->glFragmentShaderName, inputPrimitiveMode, program->expectedOutputPrimitiveCount);

	return true;
}

/**
 * Outputs the shader program's vertex attribute locations (the same values as `glGetAttribLocation()`).
 *
 * If necessary, this function also compiles, links, and uploads the program.
 *
 * @param shader The shader to query.
 * @param inputPrimitiveMode The shader program mode to query.
 * @param glContext An OpenGL context to use.
 * @param[out] positionLocation Outputs the shader program's vertex position attribute location.  Pass `NULL` if you don't care.
 * @param[out] normalLocation Outputs the shader program's vertex normal attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @param[out] tangentLocation Outputs the shader program's vertex tangent attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @param[out] bitangentLocation Outputs the shader program's vertex bitangent attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @param[out] textureCoordinateLocation Outputs the shader program's vertex texture coordinate attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @return `false` if the shader is NULL, or if it doesn't support the specified `primitiveMode`.
 *
 * @threadAnyGL
 */
bool VuoShader_getAttributeLocations(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, int *positionLocation, int *normalLocation, int *tangentLocation, int *bitangentLocation, int *textureCoordinateLocation)
{
	if (!shader)
		return false;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return false;
	}

	DEFINE_PROGRAM();

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		/// @todo cache values for each program?
		if (positionLocation)
			*positionLocation = glGetAttribLocation(program->program.programName, "position");
		if (normalLocation)
			*normalLocation = glGetAttribLocation(program->program.programName, "normal");
		if (tangentLocation)
			*tangentLocation = glGetAttribLocation(program->program.programName, "tangent");
		if (bitangentLocation)
			*bitangentLocation = glGetAttribLocation(program->program.programName, "bitangent");
		if (textureCoordinateLocation)
			*textureCoordinateLocation = glGetAttribLocation(program->program.programName, "textureCoordinate");
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return true;
}

static GLuint VuoShader_perlinTexture;	///< GL texture name for the Perlin permutation table.
/**
 * Create and load a 2D texture for a combined index permutation and gradient lookup table.
 * This texture is used for 2D and 3D noise, both classic and simplex.
 *
 * Author: Stefan Gustavson (stegu@itn.liu.se) 2004, 2005, 2011
 * ("The C code in "noisevsnoise.c" is public domain.")
 */
void VuoShader_initPerlinTexture(CGLContextObj cgl_ctx)
{
	char *pixels;
	int i,j;

	int perm[256]= {151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

	int grad3[16][3] = {{0,1, 1},{ 0, 1,-1},{ 0,-1,1},{ 0,-1,-1},
						{1,0, 1},{ 1, 0,-1},{-1, 0,1},{-1, 0,-1},
						{1,1, 0},{ 1,-1, 0},{-1, 1,0},{-1,-1, 0}, // 12 cube edges
						{1,0,-1},{-1, 0,-1},{ 0,-1,1},{ 0, 1, 1}}; // 4 more to make 16

	glGenTextures(1, &VuoShader_perlinTexture);
	glBindTexture(GL_TEXTURE_2D, VuoShader_perlinTexture);

	pixels = (char*)malloc( 256*256*4 );
	for(i = 0; i<256; i++)
		for(j = 0; j<256; j++) {
			int offset = (i*256+j)*4;
			char value = perm[(j+perm[i]) & 0xFF];
			pixels[offset+2] = grad3[value & 0x0F][0] * 64 + 64; // Gradient x
			pixels[offset+1] = grad3[value & 0x0F][1] * 64 + 64; // Gradient y
			pixels[offset+0] = grad3[value & 0x0F][2] * 64 + 64; // Gradient z
			pixels[offset+3] = value;                     // Permuted index
		}

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	free(pixels);
}

static GLuint VuoShader_gradTexture;	///< GL texture name for the Simplex permutation table.
/**
 * Create and load a 2D texture for a 4D gradient lookup table. This is used for 4D noise only.
 *
 * Author: Stefan Gustavson (stegu@itn.liu.se) 2004, 2005, 2011
 * ("The C code in "noisevsnoise.c" is public domain.")
 */
void VuoShader_initGradTexture(CGLContextObj cgl_ctx)
{
	char *pixels;
	int i,j;

	int perm[256]= {151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

	int grad4[32][4]= {{ 0, 1,1,1}, { 0, 1, 1,-1}, { 0, 1,-1,1}, { 0, 1,-1,-1}, // 32 tesseract edges
					   { 0,-1,1,1}, { 0,-1, 1,-1}, { 0,-1,-1,1}, { 0,-1,-1,-1},
					   { 1, 0,1,1}, { 1, 0, 1,-1}, { 1, 0,-1,1}, { 1, 0,-1,-1},
					   {-1, 0,1,1}, {-1, 0, 1,-1}, {-1, 0,-1,1}, {-1, 0,-1,-1},
					   { 1, 1,0,1}, { 1, 1, 0,-1}, { 1,-1, 0,1}, { 1,-1, 0,-1},
					   {-1, 1,0,1}, {-1, 1, 0,-1}, {-1,-1, 0,1}, {-1,-1, 0,-1},
					   { 1, 1,1,0}, { 1, 1,-1, 0}, { 1,-1, 1,0}, { 1,-1,-1, 0},
					   {-1, 1,1,0}, {-1, 1,-1, 0}, {-1,-1, 1,0}, {-1,-1,-1, 0}};

	glGenTextures(1, &VuoShader_gradTexture);
	glBindTexture(GL_TEXTURE_2D, VuoShader_gradTexture);

	pixels = (char*)malloc( 256*256*4 );
	for(i = 0; i<256; i++)
		for(j = 0; j<256; j++) {
			int offset = (i*256+j)*4;
			char value = perm[(j+perm[i]) & 0xFF];
			pixels[offset+2] = grad4[value & 0x1F][0] * 64 + 64; // Gradient x
			pixels[offset+1] = grad4[value & 0x1F][1] * 64 + 64; // Gradient y
			pixels[offset+0] = grad4[value & 0x1F][2] * 64 + 64; // Gradient z
			pixels[offset+3] = grad4[value & 0x1F][3] * 64 + 64; // Gradient w
		}

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	free(pixels);
}

typedef std::map<VuoGlContext, GLuint> VuoShaderContextType;	///< Type for VuoShaderContextMap.
static VuoShaderContextType VuoShaderContextMap;	///< The currently-active shader on each context.
static dispatch_semaphore_t VuoShaderContext_semaphore;	///< Serializes access to VuoShaderContextMap.
/**
 * Initializes VuoShaderContext_semaphore.
 */
static void __attribute__((constructor)) VuoShaderContext_init(void)
{
	VuoShaderContext_semaphore = dispatch_semaphore_create(1);
}

/**
 * Activates the shader program (`glUseProgram()`) on the specified `glContext`,
 * binds the shader's images to texture units, and uploads its unforms,
 * so that the shader is ready for use in rendering.
 *
 * @param shader The shader to activate.
 * @param inputPrimitiveMode The shader program mode to activate.
 * @param glContext The OpenGL context on which to activate the shader program.
 * @param outputProgram The OpenGL program name and metadata.
 * @return True if the shader is ready to use, or false if:
 *    - the shader is NULL
 *    - or `inputPrimitiveMode` is invalid
 *    - or if there is no shader for this `inputPrimitiveMode`
 *
 * @threadAnyGL
 */
bool VuoShader_activate(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, VuoGlProgram *outputProgram)
{
	if (!shader)
		return false;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
		VUserLog("Error: '%s' doesn't have a program for inputPrimitiveMode '%s'.", shader->name, VuoMesh_cStringForElementAssemblyMethod(inputPrimitiveMode));
		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return false;
	}

	DEFINE_PROGRAM();
//	VLog("Rendering %s with '%s'", VuoMesh_cStringForElementAssemblyMethod(epm), shader->name);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		VuoGlProgram_lock(program->program.programName);

		bool alreadyActiveOnThisContext = false;
		dispatch_semaphore_wait(VuoShaderContext_semaphore, DISPATCH_TIME_FOREVER);
		{
			VuoMesh_ElementAssemblyMethod epm = VuoMesh_getExpandedPrimitiveMode(inputPrimitiveMode);
			VuoShaderContextType::iterator i = VuoShaderContextMap.find(glContext);
			if (i != VuoShaderContextMap.end())
			{
				if (i->second == program->program.programName)
					alreadyActiveOnThisContext = true;
				else
					i->second = program->program.programName;
			}
			else
				VuoShaderContextMap[glContext] = program->program.programName;
		}
		dispatch_semaphore_signal(VuoShaderContext_semaphore);

		if (!alreadyActiveOnThisContext)
		{
			if (VuoIsDebugEnabled())
			{
				glValidateProgram(program->program.programName);

				int infologLength = 0;
				int charsWritten  = 0;
				char *infoLog;

				glGetProgramiv(program->program.programName, GL_INFO_LOG_LENGTH, &infologLength);

				if (infologLength > 0)
				{
					infoLog = (char *)malloc(infologLength);
					glGetProgramInfoLog(program->program.programName, infologLength, &charsWritten, infoLog);
					VUserLog("%s", infoLog);
					free(infoLog);
				}
			}

			glUseProgram(program->program.programName);
		}

		GLuint textureUnit = 0;
		bool explicitColorBuffer = false;
		for (unsigned int i = 0; i < shader->uniformsCount; ++i)
		{
			VuoShaderUniform uniform = shader->uniforms[i];
			GLint location = VuoGlProgram_getUniformLocation(program->program, uniform.name);
			if (location == -1)
				continue;

			if (strcmp(uniform.type, "VuoImage") == 0)
			{
				VuoImage image = uniform.value.image;

				glActiveTexture(GL_TEXTURE0 + textureUnit);
				if (image)
					glBindTexture(image->glTextureTarget, image->glTextureName);
				else
				{
					// When passing a null image to a shader,
					// allocate a texture unit for it anyway and unbind all textures from it,
					// so it samples the void instead of sampling
					// whatever happens to be hanging out on on the texture unit associated with the sampler.
					// https://b33p.net/kosada/node/8976
					glBindTexture(GL_TEXTURE_2D, 0);
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				}
				glUniform1i(location, textureUnit);
				++textureUnit;

				if (strcmp(uniform.name, "colorBuffer") == 0)
					explicitColorBuffer = true;
			}
			else if (strcmp(uniform.type, "VuoBoolean") == 0)
				glUniform1i(location, uniform.value.boolean);
			else if (strcmp(uniform.type, "VuoInteger") == 0)
				glUniform1i(location, uniform.value.integer);
			else if (strcmp(uniform.type, "VuoReal") == 0)
				glUniform1f(location, uniform.value.real);
			else if (strcmp(uniform.type, "VuoPoint2d") == 0)
				glUniform2f(location, uniform.value.point2d.x, uniform.value.point2d.y);
			else if (strcmp(uniform.type, "VuoPoint3d") == 0)
				glUniform3f(location, uniform.value.point3d.x, uniform.value.point3d.y, uniform.value.point3d.z);
			else if (strcmp(uniform.type, "VuoPoint4d") == 0)
				glUniform4f(location, uniform.value.point4d.x, uniform.value.point4d.y, uniform.value.point4d.z, uniform.value.point4d.w);
			else if (strcmp(uniform.type, "VuoColor") == 0)
				glUniform4f(location, uniform.value.color.r, uniform.value.color.g, uniform.value.color.b, uniform.value.color.a);
			else
				VUserLog("Error: Unknown type %s for '%s'", uniform.type, uniform.name);
		}


		GLint perlinTextureUniform = VuoGlProgram_getUniformLocation(program->program, "perlinTexture");
		if (perlinTextureUniform != -1)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);

			static dispatch_once_t initPerlinTexture = 0;
			dispatch_once(&initPerlinTexture, ^{
							  VuoShader_initPerlinTexture(cgl_ctx);
						  });

			glBindTexture(GL_TEXTURE_2D, VuoShader_perlinTexture);
			glUniform1i(perlinTextureUniform, textureUnit);
			++textureUnit;
		}

		GLint gradTextureUniform = VuoGlProgram_getUniformLocation(program->program, "gradTexture");
		if (gradTextureUniform != -1)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);

			static dispatch_once_t initGradTexture = 0;
			dispatch_once(&initGradTexture, ^{
							  VuoShader_initGradTexture(cgl_ctx);
						  });

			glBindTexture(GL_TEXTURE_2D, VuoShader_gradTexture);
			glUniform1i(gradTextureUniform, textureUnit);
			++textureUnit;
		}


		GLint colorBufferUniform = VuoGlProgram_getUniformLocation(program->program, "colorBuffer");
		GLint depthBufferUniform = VuoGlProgram_getUniformLocation(program->program, "depthBuffer");

		GLint width, height;
		if ((!explicitColorBuffer && colorBufferUniform != -1) || depthBufferUniform != -1)
		{
			GLint viewport[4];
			glGetIntegerv(GL_VIEWPORT, viewport);
			width  = viewport[2];
			height = viewport[3];
		}

		if (!explicitColorBuffer && colorBufferUniform != -1)
		{
			// Capture the context's current color image and feed it to the shader.
			glActiveTexture(GL_TEXTURE0 + textureUnit);

			GLuint colorBufferTexture = VuoGlTexturePool_use(cgl_ctx, GL_RGBA, width, height, GL_BGRA);
			glBindTexture(GL_TEXTURE_2D, colorBufferTexture);

			GLint multisampling, multisampledFramebuffer;
			glGetIntegerv(GL_SAMPLE_BUFFERS, &multisampling);
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &multisampledFramebuffer);
			if (multisampling && multisampledFramebuffer)
			{
				GLuint resolvedFramebuffer;
				glGenFramebuffers(1, &resolvedFramebuffer);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedFramebuffer);

				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTexture, 0);

					glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, multisampledFramebuffer);
					glReadBuffer(GL_COLOR_ATTACHMENT0);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					glBlitFramebuffer(0, 0, width, height,
									  0, 0, width, height,
									  GL_COLOR_BUFFER_BIT,
									  GL_NEAREST);

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				}

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, multisampledFramebuffer);
				glDeleteFramebuffers(1, &resolvedFramebuffer);
			}
			else
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, width, height, 0);

			shader->colorBuffer = VuoImage_make(colorBufferTexture, GL_RGBA, width, height);
			VuoRetain(shader->colorBuffer);

			glUniform1i(colorBufferUniform, textureUnit);
			++textureUnit;
		}

		if (depthBufferUniform != -1)
		{
			// Capture the context's current depth image and feed it to the shader.
			glActiveTexture(GL_TEXTURE0 + textureUnit);

			GLuint depthBufferTexture = VuoGlTexturePool_use(cgl_ctx, GL_DEPTH_COMPONENT16, width, height, GL_DEPTH_COMPONENT);
			glBindTexture(GL_TEXTURE_2D, depthBufferTexture);

			/// @todo support multisampled framebuffers (like the color image case above)
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 0, 0, width, height, 0);

			shader->depthBuffer = VuoImage_make(depthBufferTexture, GL_DEPTH_COMPONENT16, width, height);
			VuoRetain(shader->depthBuffer);

			glUniform1i(depthBufferUniform, textureUnit);
			++textureUnit;
		}
	}

	//	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock); --- hold the lock while the shader is active
	*outputProgram = program->program;
	return true;
}

/**
 * Deactivates the shader program on the specified `glContext`,
 * and unbinds the shader's images from their texture units.
 *
 * @param shader The shader to deactivate.
 * @param inputPrimitiveMode The shader program mode to deactivate.
 * @param glContext The OpenGL context on which to deactivate the shader program.
 *
 * @threadAnyGL
 */
void VuoShader_deactivate(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext)
{
	if (!shader)
		return;

//	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER); --- the lock is held while the shader is active
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
//		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return;
	}

	DEFINE_PROGRAM();

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		GLuint textureUnit = 0;
		for (unsigned int i = 0; i < shader->uniformsCount; ++i)
		{
			GLint location = VuoGlProgram_getUniformLocation(program->program, shader->uniforms[i].name);
			if (location == -1)
				continue;

			if (strcmp(shader->uniforms[i].type, "VuoImage") == 0)
			{
				VuoImage image = shader->uniforms[i].value.image;
				if (!image)
					continue;

				glActiveTexture(GL_TEXTURE0 + textureUnit);
				glBindTexture(image->glTextureTarget, 0);
				++textureUnit;
			}
		}

		if (VuoGlProgram_getUniformLocation(program->program, "perlinTexture") != -1)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
			++textureUnit;
		}

		if (VuoGlProgram_getUniformLocation(program->program, "gradTexture") != -1)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
			++textureUnit;
		}

		if (shader->colorBuffer)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
			++textureUnit;
			VuoRelease(shader->colorBuffer);
			shader->colorBuffer = NULL;
		}

		if (shader->depthBuffer)
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
			++textureUnit;
			VuoRelease(shader->depthBuffer);
			shader->depthBuffer = NULL;
		}

		// Instead of deactivating the shader, keep it active in hope that we can reuse it during this context's next draw call.
//		glUseProgram(0);

		VuoGlProgram_unlock(program->program.programName);
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return;
}

/**
 * Returns the context's shader state to normal (and clears VuoShader's internal cache).
 *
 * Call this when you're done using the context — after @ref VuoShader_deactivate and before @ref VuoGlContext_disuse.
 */
void VuoShader_cleanupContext(VuoGlContext glContext)
{
	dispatch_semaphore_wait(VuoShaderContext_semaphore, DISPATCH_TIME_FOREVER);
	{
		VuoShaderContextType::iterator i = VuoShaderContextMap.find(glContext);
		if (i != VuoShaderContextMap.end())
			VuoShaderContextMap.erase(i);
	}
	dispatch_semaphore_signal(VuoShaderContext_semaphore);

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	glUseProgram(0);
}

/**
 * Decodes the JSON object @c js, expected to contain a 64-bit integer (memory address or 0), to create a new @c VuoShader.
 *
 * @threadAny
 */
VuoShader VuoShader_makeFromJson(json_object *js)
{
	if (!js)
		return NULL;

	VuoShader s = NULL;

	if (json_object_get_type(js) == json_type_int)
		s = (VuoShader) json_object_get_int64(js);

	if (!s)
		return VuoShader_makeDefaultShader();

	return s;
}

/**
 * Encodes @c value as a JSON object.
 *
 * Serializes the pointer to the VuoShader object, since we need to preserve its reference count.
 *
 * @threadAny
 */
json_object * VuoShader_getJson(const VuoShader value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * Returns a summary of the shader: the text description provided to @ref VuoShader_make.
 *
 * @threadAny
 */
char * VuoShader_getSummary(const VuoShader value)
{
	if (!value)
		return strdup("(no shader)");

	return strdup(value->name);
}

/**
 * Converts the provided `vuoCoordinates` into GLSL Sampler Coordinates relative to the provided `image`.
 *
 * Vuo Coordinates range from (-1,-1/aspectRatio) in the bottom left to (1,1/aspectRatio) in the top right.
 *
 * GLSL Sampler Coordinates range from (0,0) to (1,1).
 *
 * If `image` is rendered in a scene, centered at (0,0,0), width 2, at its correct aspect ratio,
 * this function will transform 2D coordinates along the XY plane (at Z=0) into correct sampler coordinates for that image.
 *
 * @threadAny
 */
VuoPoint2d VuoShader_samplerCoordinatesFromVuoCoordinates(VuoPoint2d vuoCoordinates, VuoImage image)
{
	VuoPoint2d samplerCoordinates;
	samplerCoordinates.x = VuoShader_samplerSizeFromVuoSize(vuoCoordinates.x) + 0.5;
	samplerCoordinates.y = VuoShader_samplerSizeFromVuoSize(vuoCoordinates.y)/((double)image->pixelsHigh/image->pixelsWide) + 0.5;
	return samplerCoordinates;
}

/**
 * Converts an x-axis distance in Vuo Coordinates into GLSL Sampler Coordinates.  (Divides by 2.)
 *
 * @threadAny
 */
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize)
{
	return vuoSize/2.;
}

#include "VuoShaderShaders.h"
#include "VuoShaderUniforms.h"
