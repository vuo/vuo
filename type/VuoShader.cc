/**
 * @file
 * VuoShader implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>

#include "node.h"
#include "type.h"

extern "C"
{
#include "VuoShader.h"
#include "VuoGlPool.h"
#include "VuoTime.h"

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
						"VuoTime",
						"VuoList_VuoBoolean",
						"VuoList_VuoInteger",
						"VuoList_VuoImage",
						"VuoList_VuoColor",
						"VuoList_VuoPoint2d",
						"VuoList_VuoPoint3d",
						"VuoList_VuoPoint4d",
						"VuoList_VuoReal",
						"VuoList_VuoText",
						"VuoGlContext",
						"VuoGlPool",
						"OpenGL.framework"
					 ]
				 });
#endif
/// @}
}

#include "VuoShaderIssues.hh"

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

		if (strcmp(s->uniforms[u].type, "VuoImage") == 0
		 || strncmp(s->uniforms[u].type, "VuoList_", 8) == 0
		 || strcmp(s->uniforms[u].type, "mat2") == 0
		 || strcmp(s->uniforms[u].type, "mat3") == 0
		 || strcmp(s->uniforms[u].type, "mat4") == 0)
			// It's equivalent to release any pointer in the union.
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
	t->useAlphaAsCoverage = false;

	t->colorBuffer = NULL;
	t->depthBuffer = NULL;

	t->activationCount = 0;
	t->lastActivationTime = 0;

	t->lock = dispatch_semaphore_create(1);

	return t;
}

/**
 * Creates a shader object from a @ref VuoShaderFile.
 *
 * Don't call @ref VuoShader_addSource;
 * the sources are automatically retrieved from the VuoShaderFile.
 *
 * @threadAny
 * @version200New
 */
VuoShader VuoShader_makeFromFile(VuoShaderFile *shaderFile)
{
	VuoShader t = VuoShader_make(shaderFile->name().c_str());

	VuoShader_addSource(t, VuoMesh_IndividualTriangles,
						shaderFile->expandedVertexSource().c_str(),
						shaderFile->expandedGeometrySource().c_str(),
						shaderFile->expandedFragmentSource().c_str());

	/// @todo set geometry shader properties

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
	if (!shader)
		return VuoShader_makeDefaultShader();
	else
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
 *     attribute vec3 position;  // required
 *     attribute vec3 normal;
 *     attribute vec2 textureCoordinate;
 *     attribute vec4 vertexColor;
 *     uniform bool hasTextureCoordinates;
 *     uniform bool hasVertexColors;
 *
 * @ref VuoSceneObjectRenderer renders a @ref VuoSceneObject into a @ref VuoSceneObject,
 * and automatically provides several uniform values and vertex attributes to shaders:
 *
 *     uniform mat4 modelviewMatrix;
 *     uniform mat4 modelviewMatrixInverse;
 *     attribute vec3 position;
 *     attribute vec3 normal;
 *     attribute vec2 textureCoordinate;
 *     attribute vec4 vertexColor;
 *
 * And it expects as output:
 *
 *     varying vec3 outPosition;
 *     varying vec3 outNormal;
 *     varying vec2 outTextureCoordinate;
 *     varying vec4 outVertexColor;
 *
 * @threadAny
 */
void VuoShader_addSource(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, const char *vertexShaderSource, const char *geometryShaderSource, const char *fragmentShaderSource)
{
	if (!shader)
		return;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();

	if (!VuoText_isEmpty(vertexShaderSource))
		program->vertexSource = VuoText_make(vertexShaderSource);
	else
	{
		const char *defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
			\n#include "VuoGlslProjection.glsl"

			// Inputs
			uniform mat4 modelviewMatrix;
			attribute vec3 position;
			attribute vec2 textureCoordinate;

			// Outputs to fragment shader
			varying vec2 fragmentTextureCoordinate;

			void main()
			{
				fragmentTextureCoordinate = textureCoordinate;

				gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));
			}
		);
		const char *defaultVertexShaderWithColorSource = VUOSHADER_GLSL_SOURCE(120,
			\n#include "VuoGlslProjection.glsl"

			// Inputs
			uniform mat4 modelviewMatrix;
			attribute vec3 position;
			attribute vec2 textureCoordinate;
			attribute vec4 vertexColor;
			uniform bool hasVertexColors;

			// Outputs to fragment shader
			varying vec2 fragmentTextureCoordinate;
			varying vec4 fragmentVertexColor;

			void main()
			{
				fragmentTextureCoordinate = textureCoordinate;
				fragmentVertexColor = hasVertexColors ? vertexColor : vec4(1.);

				gl_Position = VuoGlsl_projectPosition(modelviewMatrix * vec4(position, 1.));
			}
		);

		if (strstr(fragmentShaderSource, "fragmentVertexColor"))
			program->vertexSource = VuoText_make(defaultVertexShaderWithColorSource);
		else
			program->vertexSource = VuoText_make(defaultVertexShaderSource);
	}
	VuoRetain(program->vertexSource);

	if (!VuoText_isEmpty(geometryShaderSource))
	{
		program->geometrySource = VuoText_make(geometryShaderSource);
		VuoRetain(program->geometrySource);
	}

	if (!VuoText_isEmpty(fragmentShaderSource))
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
 * See `VuoShader::isTransparent`.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setTransparent(VuoShader shader, const bool isTransparent)
{
	if (!shader)
		return;

	shader->isTransparent = isTransparent;
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
 * Converts
 * `gl_FragColor = beforeFunction(inputImage, isf_FragNormCoord.xy);`
 * to
 * `gl_FragColor = afterFunction2D(inputImage, _inputImage_imgRect, _inputImage_imgSize, _inputImage_flip, isf_FragNormCoord.xy);`
 * or `afterFunctionRect`, depending on the sampler type.
 *
 * If `isThis` is true, the function is expected to have 1 argument (the texture coordinate is implicit).
 */
static void VuoShader_replaceImageMacro(VuoShader shader, string &source, map<string, GLint> &imagesToDeclare, string beforeFunction, bool isThis, string afterFunction2D, string afterFunctionRect, VuoShaderFile::Stage stage, VuoShaderIssues *outIssues)
{
	string::size_type beforeFunctionLength = beforeFunction.length();
	string::size_type pos = 0;
	while ( (pos = source.find(beforeFunction, pos)) != string::npos )
	{
		size_t offset = pos + beforeFunctionLength;

		while (isspace(source[offset]))
			++offset;

		if (source[offset] != '(')
		{
			/// @todo calculate line number
			if (outIssues)
				outIssues->addIssue(stage, VuoShaderIssues::NoLine, "Syntax error in " + beforeFunction + ": expected '('.");
			else
				VUserLog("Syntax error in %s: expected '('.", beforeFunction.c_str());
			pos += beforeFunctionLength;
			continue;
		}
		++offset;

		size_t samplerStart = offset;
		char samplerEndChar = isThis ? ')' : ',';
		while (source[offset] != samplerEndChar)
			++offset;
		size_t samplerEnd = offset;

		string sampler = source.substr(samplerStart, samplerEnd - samplerStart);

		if (isThis)
			source.insert(samplerEnd, ", isf_FragNormCoord");
		else
			source.insert(samplerEnd,
						  ", _" + sampler + "_imgRect"
						+ ", _" + sampler + "_imgSize"
						+ ", _" + sampler + "_flip");

		string replacement = afterFunction2D;
		GLint target;
		for (int i = 0; i < shader->uniformsCount; ++i)
			if (strcmp(shader->uniforms[i].type, "VuoImage") == 0
			 && shader->uniforms[i].name == sampler
			 && shader->uniforms[i].value.image)
			{
				target = shader->uniforms[i].value.image->glTextureTarget;
				if (shader->uniforms[i].value.image->glTextureTarget == GL_TEXTURE_RECTANGLE_EXT)
					replacement = afterFunctionRect;
				shader->uniforms[i].compiledTextureTarget = target;
				break;
			}

		source.replace(pos, beforeFunctionLength, replacement);

		imagesToDeclare.insert(std::make_pair(sampler, target));
	}
}

/**
 * Replaces `IMG_SIZE(someImage)` with `_someImage_imgSize`.
 */
static void VuoShader_replaceSizeMacro(VuoShader shader, string &source, string before, string after)
{
	string prefix(before + "(");
	string::size_type prefixLength = prefix.length();
	string::size_type pos = 0;
	while ( (pos = source.find(prefix, pos)) != string::npos )
	{
		size_t offset = pos + prefixLength;
		size_t samplerStart = offset;
		while (source[offset] != ')')
			++offset;
		size_t samplerEnd = offset;
		string sampler = source.substr(samplerStart, samplerEnd - samplerStart);
		source.replace(pos, samplerEnd - samplerStart + prefixLength + 1, "_" + sampler + "_" + after);
	}
}

/**
 * Replaces the `IMG_PIXEL` and `IMG_NORM_PIXEL` macros with the appropriate function call
 * depending on the image's OpenGL target, and fill in the placeholder image declarations.
 */
static void VuoShader_replaceImageMacros(VuoShader shader, string &source, VuoShaderFile::Stage stage, VuoShaderIssues *outIssues)
{
	map<string, GLint> imagesToDeclare;
	VuoShader_replaceImageMacro(shader, source, imagesToDeclare, "IMG_PIXEL",           false, "VVSAMPLER_2DBYPIXEL", "VVSAMPLER_2DRECTBYPIXEL", stage, outIssues);
	VuoShader_replaceImageMacro(shader, source, imagesToDeclare, "IMG_NORM_PIXEL",      false, "VVSAMPLER_2DBYNORM",  "VVSAMPLER_2DRECTBYNORM",  stage, outIssues);
	VuoShader_replaceImageMacro(shader, source, imagesToDeclare, "IMG_THIS_PIXEL",      true,  "texture2D", "texture2DRect", stage, outIssues);
	VuoShader_replaceImageMacro(shader, source, imagesToDeclare, "IMG_THIS_NORM_PIXEL", true,  "texture2D", "texture2DRect", stage, outIssues);

	VuoShader_replaceSizeMacro(shader, source, "IMG_SIZE",    "imgSize");
	VuoShader_replaceSizeMacro(shader, source, "LIST_LENGTH", "length");

	// Fill in image declaration placeholders.
	for (map<string, GLint>::iterator it = imagesToDeclare.begin(); it != imagesToDeclare.end(); ++it)
	{
		string samplerType;
		if (it->second == GL_TEXTURE_RECTANGLE_EXT)
			samplerType = "sampler2DRect";
		else // if (target == GL_TEXTURE_2D)
			samplerType = "sampler2D";

		string placeholder = "//uniform VuoImage " + it->first;
		string replacement = "uniform " + samplerType + " " + it->first;

		string::size_type pos = source.find(placeholder);
		if (pos == string::npos)
		{
			if (outIssues)
				outIssues->addIssue(stage, VuoShaderIssues::NoLine, "Unknown image \"" + it->first + "\".");
			else
				VUserLog("Unknown image \"%s\".", it->first.c_str());
			continue;
		}
		source.replace(pos, placeholder.length(), replacement);
	}
}

/**
 * Ensures that the source code for the specified `inputPrimitiveMode` is compiled, linked, and uploaded.
 * If the shader is NULL, or there is no source code for the specified `inputPrimitiveMode`, or it fails to compile or link, returns `false`.
 *
 * Must be called while `shader->lock` is locked.
 */
static bool VuoShader_ensureUploaded(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, VuoShaderIssues *outIssues)
{
	if (!shader)
		return false;

	DEFINE_PROGRAM();

	// Is the shader already compiled/linked/uploaded?
	if (program->program.programName)
	{
		// If the shader is already compiled/linked/uploaded,
		// are its image targets up-to-date with the current image uniforms?

		bool upToDate = true;
		for (unsigned int i = 0; i < shader->uniformsCount; ++i)
			if (strcmp(shader->uniforms[i].type, "VuoImage") == 0)
				if (shader->uniforms[i].value.image
				 && shader->uniforms[i].compiledTextureTarget
				 && shader->uniforms[i].value.image->glTextureTarget != shader->uniforms[i].compiledTextureTarget)
				{
					upToDate = false;
					break;
				}

		if (upToDate)
			return true;
	}


	// Is there source code available?
	// By this point, if no vertex shader was provided, the default vertex shader should already have been filled in.
	if (!program->vertexSource)
		return false;

	// If we previously attempted to compile this subshader and it failed, don't try again.
	if (program->compilationAttempted)
		return false;
	program->compilationAttempted = true;

	string vertexSource = program->vertexSource;
	VuoShader_replaceImageMacros(shader, vertexSource, VuoShaderFile::Vertex, outIssues);
	program->glVertexShaderName = VuoGlShader_use(glContext, GL_VERTEX_SHADER, vertexSource.c_str(), static_cast<void *>(outIssues));
	if (!program->glVertexShaderName)
		return false;

	if (!VuoText_isEmpty(program->geometrySource))
	{
		string geometrySource = program->geometrySource;
		VuoShader_replaceImageMacros(shader, geometrySource, VuoShaderFile::Geometry, outIssues);
		program->glGeometryShaderName = VuoGlShader_use(glContext, GL_GEOMETRY_SHADER_EXT, geometrySource.c_str(), static_cast<void *>(outIssues));
		if (!program->glGeometryShaderName)
			return false;
	}

	if (!VuoText_isEmpty(program->fragmentSource))
	{
		string fragmentSource = program->fragmentSource;
		VuoShader_replaceImageMacros(shader, fragmentSource, VuoShaderFile::Fragment, outIssues);
		program->glFragmentShaderName = VuoGlShader_use(glContext, GL_FRAGMENT_SHADER, fragmentSource.c_str(), static_cast<void *>(outIssues));
		if (!program->glFragmentShaderName)
			return false;
	}

	program->program = VuoGlProgram_use(glContext, shader->name, program->glVertexShaderName, program->glGeometryShaderName, program->glFragmentShaderName, inputPrimitiveMode, program->expectedOutputPrimitiveCount, static_cast<void *>(outIssues));

	return program->program.programName > 0 ? true : false;
}

/**
 * Compiles and uploads the shader, outputting any issues in a @ref VuoShaderIssues instance.
 *
 * This is optional; the shader will automatically be compiled and uploaded when needed.
 * It's only necessary if you want to get the compilation warnings/errors.
 *
 * @return `false` if the shader is NULL, or if it doesn't support the specified `primitiveMode`, or if the shader fails to compile or link.
 *
 * @threadAnyGL
 * @version200New
 */
bool VuoShader_upload(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, void *outIssues)
{
	if (!shader)
		return false;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext, static_cast<VuoShaderIssues *>(outIssues)))
	{
		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return false;
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
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
 * @param[out] textureCoordinateLocation Outputs the shader program's vertex texture coordinate attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @param[out] colorLocation Outputs the shader program's vertex color attribute location (or -1 if this shader program doesn't have one).  Pass `NULL` if you don't care.
 * @return `false` if the shader is NULL, or if it doesn't support the specified `primitiveMode`.
 *
 * @threadAnyGL
 */
bool VuoShader_getAttributeLocations(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, int *positionLocation, int *normalLocation, int *textureCoordinateLocation, int *colorLocation)
{
	if (!shader)
		return false;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext, NULL))
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
		if (textureCoordinateLocation)
			*textureCoordinateLocation = glGetAttribLocation(program->program.programName, "textureCoordinate");
		if (colorLocation)
			*colorLocation = glGetAttribLocation(program->program.programName, "vertexColor");
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

	VuoShader_perlinTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_NoAllocation, GL_TEXTURE_2D, GL_RGBA, 256, 256, GL_BGRA, NULL);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

	VuoShader_gradTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_NoAllocation, GL_TEXTURE_2D, GL_RGBA, 256, 256, GL_BGRA, NULL);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	free(pixels);
}

typedef std::map<VuoGlContext, GLuint> VuoShaderContextType;	///< Type for VuoShaderContextMap.
extern VuoShaderContextType VuoShaderContextMap;
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
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext, NULL))
	{
		VUserLog("Error: '%s' doesn't have a program for inputPrimitiveMode '%s'.", shader->name, VuoMesh_cStringForElementAssemblyMethod(inputPrimitiveMode));
		dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
		return false;
	}

	DEFINE_PROGRAM();
//	VLog("Rendering %s with '%s'", VuoMesh_cStringForElementAssemblyMethod(epm), shader->name);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		bool alreadyActiveOnThisContext = false;
		dispatch_semaphore_wait(VuoShaderContext_semaphore, DISPATCH_TIME_FOREVER);
		{
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
				glGetProgramiv(program->program.programName, GL_INFO_LOG_LENGTH, &infologLength);
				if (infologLength > 0)
				{
					char *infoLog = (char *)malloc(infologLength);
					int charsWritten  = 0;
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


			if (strncmp(uniform.type, "VuoList_", 8) == 0)
			{
				GLint location = VuoGlProgram_getUniformLocation(program->program, (string(uniform.name) + "[0]").c_str());
				if (location == -1)
				{
					VDebugLog("Warning: Shader '%s' has a value for '%s', but the linked program has no uniform by that name.", shader->name, uniform.name);
					continue;
				}

				size_t itemCount;
				if (strcmp(uniform.type, "VuoList_VuoBoolean") == 0)
				{
					itemCount = VuoListGetCount_VuoBoolean(uniform.value.booleans);
					VuoBoolean *itemData = VuoListGetData_VuoBoolean(uniform.value.booleans);
					GLint *itemDataGL = (GLint *)malloc(sizeof(GLint) * itemCount);
					for (size_t i = 0; i < itemCount; ++i)
						itemDataGL[i] = itemData[i];
					glUniform1iv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoInteger") == 0)
				{
					itemCount = VuoListGetCount_VuoInteger(uniform.value.integers);
					VuoInteger *itemData = VuoListGetData_VuoInteger(uniform.value.integers);
					GLint *itemDataGL = (GLint *)malloc(sizeof(GLint) * itemCount);
					for (size_t i = 0; i < itemCount; ++i)
						itemDataGL[i] = itemData[i];
					glUniform1iv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoReal") == 0)
				{
					itemCount = VuoListGetCount_VuoReal(uniform.value.reals);
					VuoReal *itemData = VuoListGetData_VuoReal(uniform.value.reals);
					GLfloat *itemDataGL = (GLfloat *)malloc(sizeof(GLfloat) * itemCount);
					for (size_t i = 0; i < itemCount; ++i)
						itemDataGL[i] = itemData[i];
					glUniform1fv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoPoint2d") == 0)
				{
					itemCount = VuoListGetCount_VuoPoint2d(uniform.value.point2ds);
					VuoPoint2d *itemData = VuoListGetData_VuoPoint2d(uniform.value.point2ds);
					GLfloat *itemDataGL = (GLfloat *)malloc(sizeof(GLfloat) * itemCount * 2);
					for (size_t i = 0; i < itemCount; ++i)
					{
						itemDataGL[i*2 + 0] = itemData[i].x;
						itemDataGL[i*2 + 1] = itemData[i].y;
					}
					glUniform2fv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoPoint3d") == 0)
				{
					itemCount = VuoListGetCount_VuoPoint3d(uniform.value.point3ds);
					VuoPoint3d *itemData = VuoListGetData_VuoPoint3d(uniform.value.point3ds);
					GLfloat *itemDataGL = (GLfloat *)malloc(sizeof(GLfloat) * itemCount * 3);
					for (size_t i = 0; i < itemCount; ++i)
					{
						itemDataGL[i*3 + 0] = itemData[i].x;
						itemDataGL[i*3 + 1] = itemData[i].y;
						itemDataGL[i*3 + 2] = itemData[i].z;
					}
					glUniform3fv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoPoint4d") == 0)
				{
					itemCount = VuoListGetCount_VuoPoint4d(uniform.value.point4ds);
					VuoPoint4d *itemData = VuoListGetData_VuoPoint4d(uniform.value.point4ds);
					GLfloat *itemDataGL = (GLfloat *)malloc(sizeof(GLfloat) * itemCount * 4);
					for (size_t i = 0; i < itemCount; ++i)
					{
						itemDataGL[i*4 + 0] = itemData[i].x;
						itemDataGL[i*4 + 1] = itemData[i].y;
						itemDataGL[i*4 + 2] = itemData[i].z;
						itemDataGL[i*4 + 3] = itemData[i].w;
					}
					glUniform4fv(location, itemCount, itemDataGL);
				}
				else if (strcmp(uniform.type, "VuoList_VuoColor") == 0)
				{
					itemCount = VuoListGetCount_VuoColor(uniform.value.colors);
					VuoColor *itemData = VuoListGetData_VuoColor(uniform.value.colors);
					GLfloat *itemDataGL = (GLfloat *)malloc(sizeof(GLfloat) * itemCount * 4);
					for (size_t i = 0; i < itemCount; ++i)
					{
						itemDataGL[i*4 + 0] = itemData[i].r;
						itemDataGL[i*4 + 1] = itemData[i].g;
						itemDataGL[i*4 + 2] = itemData[i].b;
						itemDataGL[i*4 + 3] = itemData[i].a;
					}
					glUniform4fv(location, itemCount, itemDataGL);
				}
				else
					VDebugLog("Warning: Shader '%s' has unknown type '%s' for uniform '%s'.", shader->name, uniform.type, uniform.name);

				GLint listLengthUniform = VuoGlProgram_getUniformLocation(program->program, (string("_") + uniform.name + "_length").c_str());
				if (listLengthUniform != -1)
					glUniform1i(listLengthUniform, itemCount);

				continue;
			}


			// Populate the ISF image-related uniforms, even if the image itself isn't used.
			if (strcmp(uniform.type, "VuoImage") == 0 && uniform.value.image)
			{
				VuoImage image = uniform.value.image;

				/// @todo https://b33p.net/kosada/node/9889
				GLint imgRectUniform = VuoGlProgram_getUniformLocation(program->program, (string("_") + uniform.name + "_imgRect").c_str());
				if (imgRectUniform != -1)
				{
					if (image->glTextureTarget == GL_TEXTURE_2D)
						glUniform4f(imgRectUniform, 0, 0, 1, 1);
					else
						glUniform4f(imgRectUniform, 0, 0, image->pixelsWide, image->pixelsHigh);
				}

				GLint imgSizeUniform = VuoGlProgram_getUniformLocation(program->program, (string("_") + uniform.name + "_imgSize").c_str());
				if (imgSizeUniform != -1)
					glUniform2f(imgSizeUniform, image->pixelsWide, image->pixelsHigh);

				/// @todo https://b33p.net/kosada/node/9889
				GLint flipUniform = VuoGlProgram_getUniformLocation(program->program, (string("_") + uniform.name + "_flip").c_str());
				if (flipUniform != -1)
					glUniform1i(flipUniform, 0);
			}

			GLint location = VuoGlProgram_getUniformLocation(program->program, uniform.name);
			if (location == -1)
			{
				VDebugLog("Warning: Shader '%s' has a value for '%s', but the linked program has no uniform by that name.", shader->name, uniform.name);
				continue;
			}

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
			else if (strcmp(uniform.type, "mat2") == 0)
				glUniformMatrix2fv(location, 1, GL_FALSE, uniform.value.mat2);
			else if (strcmp(uniform.type, "mat3") == 0)
				glUniformMatrix3fv(location, 1, GL_FALSE, uniform.value.mat3);
			else if (strcmp(uniform.type, "mat4") == 0)
				glUniformMatrix4fv(location, 1, GL_FALSE, uniform.value.mat4);
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

			GLuint colorBufferTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, GL_TEXTURE_2D, GL_RGBA, width, height, GL_BGRA, NULL);
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

			GLuint depthBufferTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, GL_TEXTURE_2D, GL_DEPTH_COMPONENT16, width, height, GL_DEPTH_COMPONENT, NULL);
			glBindTexture(GL_TEXTURE_2D, depthBufferTexture);

			/// @todo support multisampled framebuffers (like the color image case above)
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 0, 0, width, height, 0);

			shader->depthBuffer = VuoImage_make(depthBufferTexture, GL_DEPTH_COMPONENT16, width, height);
			VuoRetain(shader->depthBuffer);

			glUniform1i(depthBufferUniform, textureUnit);
			++textureUnit;
		}

		// ISF
		{
			// RENDERSIZE is provided by a #define in VuoShaderFile::insertPreamble.
			// TIME is provided by an input port.

			double now = VuoLogGetElapsedTime();
			GLint timedeltaUniform = VuoGlProgram_getUniformLocation(program->program, "TIMEDELTA");
			if (timedeltaUniform != -1)
				glUniform1f(timedeltaUniform, now - shader->lastActivationTime);
			shader->lastActivationTime = now;

			GLint dateUniform = VuoGlProgram_getUniformLocation(program->program, "DATE");
			if (dateUniform != -1)
			{
				VuoInteger year;
				VuoInteger month;
				VuoInteger dayOfMonth;
				VuoInteger hour;
				VuoInteger minute;
				VuoReal    second;
				if (VuoTime_getComponents(VuoTime_getCurrent(), &year, NULL, &month, &dayOfMonth, NULL, NULL, &hour, &minute, &second))
					glUniform4f(dateUniform, (float)year, (float)month, (float)dayOfMonth, (float)(second + (minute + (hour * 60)) * 60));
			}

			GLint frameindexUniform = VuoGlProgram_getUniformLocation(program->program, "FRAMEINDEX");
			if (frameindexUniform != -1)
				glUniform1i(frameindexUniform, shader->activationCount);
			++shader->activationCount;
		}
	}

	//	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock); --- hold the lock while the shader is active
	*outputProgram = program->program;
	return true;
}

/**
 * Unbinds the shader's images from their texture units.
 *
 * The shader program remains in use on `glContext` (in case it's needed again soon).
 * To disuse the shader, see @see VuoShader_resetContext.
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
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext, NULL))
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
	}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return;
}

/**
 * Disuses whatever shader (if any) is currently active on `glContext`.
 *
 * @threadAny
 */
void VuoShader_resetContext(VuoGlContext glContext)
{
	dispatch_semaphore_wait(VuoShaderContext_semaphore, DISPATCH_TIME_FOREVER);
	VuoShaderContextType::iterator i = VuoShaderContextMap.find(glContext);
	if (i != VuoShaderContextMap.end())
	{
		i->second = 0;
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;
		glUseProgram(0);
	}
	dispatch_semaphore_signal(VuoShaderContext_semaphore);
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

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "pointer", &o))
		return (VuoShader)json_object_get_int64(o);

	return VuoShader_makeDefaultShader();
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
	if (!value)
		return NULL;

	json_object *js = json_object_new_object();
	json_object_object_add(js, "pointer", json_object_new_int64((int64_t)value));
	return js;
}

/**
 * Calls VuoShader_getJson(). Interprocess support is not yet implemented.
 *
 * @threadAny
 */
json_object * VuoShader_getInterprocessJson(const VuoShader value)
{
	return VuoShader_getJson(value);
}

/**
 * Returns a summary of the shader: the text description provided to @ref VuoShader_make.
 *
 * @threadAny
 */
char * VuoShader_getSummary(const VuoShader value)
{
	if (!value)
		return strdup("No shader");

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

/**
 * Converts the provided `normalizedCoordinates` into GLSL sampler2DRect coordinates relative to the provided width/height.
 *
 * GLSL Normalized Sampler Coordinates range from (0,0) in the bottom left to (1,1) in the top right.
 *
 * GLSL sampler2DRect Coordinates range from (0,0) to (imageWidth,imageHeight).
 *
 * @threadAny
 * @version200New
 */
VuoPoint2d VuoShader_samplerRectCoordinatesFromNormalizedCoordinates(VuoPoint2d c, VuoInteger imageWidth, VuoInteger imageHeight)
{
	c.x *= imageWidth;
	c.y *= imageHeight;
	return c;
}

/**
 * Returns true if the shader, as configured, will produce fully opaque output.
 *
 * Returns false if:
 *
 *    - the shader is explicitly transparent (if `isTransparent` or `useAlphaAsCoverage` is set)
 *    - or the shader has one or more VuoColor uniforms whose alpha is less than 1
 *    - or the shader has one or more VuoList_VuoColor uniforms having a color whose alpha is less than 1
 *    - or the shader has a VuoReal uniform named `alpha` whose value is less than 1
 *    - or the shader has one or more VuoImage uniforms with an alpha channel
 *      (it doesn't actually check whether the image's alpha channel has less-than-1 values in it,
 *      since that would be really slow)
 *
 * @version200New
*/
bool VuoShader_isOpaque(VuoShader shader)
{
	if (!shader)
		return true;

	if (shader->isTransparent || shader->useAlphaAsCoverage)
	{
//		VLog("Shader %p ('%s') isTransparent=%d useAlphaAsCoverage=%d", shader, shader->name, shader->isTransparent, shader->useAlphaAsCoverage);
		return false;
	}

	bool opaque = true;

	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	for (int i = 0; i < shader->uniformsCount; ++i)
	{
		if (strcmp(shader->uniforms[i].type, "VuoColor") == 0
		 && !VuoColor_isOpaque(shader->uniforms[i].value.color))
		{
//			VLog("Shader %p ('%s') has a transparent color: %s", shader, shader->name, VuoColor_getShortSummary(shader->uniforms[i].value.color));
			opaque = false;
			goto done;
		}

		if (strcmp(shader->uniforms[i].type, "VuoList_VuoColor") == 0
		 && !VuoColor_areAllOpaque(shader->uniforms[i].value.colors))
		{
//			VLog("Shader %p ('%s') has a transparent color", shader, shader->name);
			opaque = false;
			goto done;
		}

		if (strcmp(shader->uniforms[i].type, "VuoReal") == 0
		 && strcmp(shader->uniforms[i].name, "alpha") == 0
		 && shader->uniforms[i].value.real < 1)
		{
//			VLog("Shader %p ('%s') has 'alpha' value %g", shader, shader->name, shader->uniforms[i].value.real);
			opaque = false;
			goto done;
		}

		if (strcmp(shader->uniforms[i].type, "VuoImage") == 0
		 && shader->uniforms[i].value.image
		 && VuoGlTexture_formatHasAlphaChannel(shader->uniforms[i].value.image->glInternalFormat))
		{
//			VLog("Shader %p ('%s') has an image with an alpha channel: %s", shader, shader->name, VuoImage_getSummary(shader->uniforms[i].value.image));
			opaque = false;
			goto done;
		}
	}

done:
	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);

	return opaque;
}

/**
 * Returns true if the shader is anything other than the default (blue/purple gradient checkerboard).
 *
 * @version200New
 */
bool VuoShader_isPopulated(VuoShader shader)
{
	if (!shader)
		return false;

	if (shader == VuoShader_makeDefaultShader())
		return false;

	return true;
}


#include "VuoShaderShaders.h"
#include "VuoShaderUniforms.h"
