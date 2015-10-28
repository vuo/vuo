/**
 * @file
 * VuoShader implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/**
 * Frees the CPU memory and GPU objects associated with the shader.
 *
 * @threadAny
 */
void VuoShader_free(void *shader)
{
	VuoShader s = (VuoShader)shader;

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		// Just delete the program, not the shaders (since @ref VuoGlShader_use expects them to persist).
		if (s->pointProgram.glProgramName)
			glDeleteProgram(s->pointProgram.glProgramName);

		if (s->lineProgram.glProgramName)
			glDeleteProgram(s->lineProgram.glProgramName);

		if (s->triangleProgram.glProgramName)
			glDeleteProgram(s->triangleProgram.glProgramName);

		VuoGlContext_disuse(cgl_ctx);
	}

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

	dispatch_release(s->lock);

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

	t->lock = dispatch_semaphore_create(1);

	return t;
}

/**
 * Defines a `program` variable and initializes it with the relevant program based on `inputPrimitiveMode`.
 */
#define DEFINE_PROGRAM()									\
	VuoSubshader *program;									\
	if (inputPrimitiveMode == VuoMesh_IndividualTriangles	\
	 || inputPrimitiveMode == VuoMesh_TriangleFan			\
	 || inputPrimitiveMode == VuoMesh_TriangleStrip)		\
		program = &shader->triangleProgram;					\
	else if (inputPrimitiveMode == VuoMesh_IndividualLines	\
		  || inputPrimitiveMode == VuoMesh_LineStrip)		\
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
 *     uniform mat4 modelviewMatrix;   // required
 *
 *     uniform vec3 cameraPosition;
 *
 *     uniform float aspectRatio;
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
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();

	if (vertexShaderSource)
		program->vertexSource = VuoText_make(vertexShaderSource);
	else
	{
		const char *defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
			// Inputs
			uniform mat4 projectionMatrix;
			uniform mat4 modelviewMatrix;
			attribute vec4 position;
			attribute vec4 textureCoordinate;

			// Outputs to fragment shader
			varying vec4 fragmentTextureCoordinate;

			void main()
			{
				fragmentTextureCoordinate = textureCoordinate;

				gl_Position = projectionMatrix * modelviewMatrix * position;
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

	dispatch_semaphore_signal(shader->lock);
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
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	program->expectedOutputPrimitiveCount = expectedOutputPrimitiveCount;

	dispatch_semaphore_signal(shader->lock);
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
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);

	DEFINE_PROGRAM();
	unsigned int expectedOutputPrimitiveCount = program->expectedOutputPrimitiveCount;

	dispatch_semaphore_signal(shader->lock);

	return expectedOutputPrimitiveCount;
}

/**
 * Returns `true` if each of `shader`'s defined `inputPrimitiveMode`s has a vertex shader (and optionally a geometry shader) but lacks a fragment shader.
 *
 * @threadAny
 */
bool VuoShader_isTransformFeedback(VuoShader shader)
{
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);

	if (shader->pointProgram.glFragmentShaderName
	 || shader->lineProgram.glFragmentShaderName
	 || shader->triangleProgram.glFragmentShaderName)
	{
		dispatch_semaphore_signal(shader->lock);
		return false;
	}

	dispatch_semaphore_signal(shader->lock);
	return true;
}

/**
 * Ensures that the source code for the specified `inputPrimitiveMode` is compiled, linked, and uploaded.
 * If there is no source code for the specified `inputPrimitiveMode`, or it fails to compile or link, returns `false`.
 *
 * Must be called while `shader->lock` is locked.
 */
bool VuoShader_ensureUploaded(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext)
{
	DEFINE_PROGRAM();

	// Is the shader already compiled/linked/uploaded?
	if (program->glProgramName)
		return true;

	// Is there source code available?
	if (!program->vertexSource)
		return false;

	program->glVertexShaderName = VuoGlShader_use(glContext, GL_VERTEX_SHADER, program->vertexSource);
	if (program->geometrySource)
		program->glGeometryShaderName = VuoGlShader_use(glContext, GL_GEOMETRY_SHADER_EXT, program->geometrySource);
	if (program->fragmentSource)
		program->glFragmentShaderName = VuoGlShader_use(glContext, GL_FRAGMENT_SHADER, program->fragmentSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		program->glProgramName = glCreateProgram();
		glAttachShader(program->glProgramName, program->glVertexShaderName);
		if (program->glGeometryShaderName)
			glAttachShader(program->glProgramName, program->glGeometryShaderName);
		if (program->glFragmentShaderName)
			glAttachShader(program->glProgramName, program->glFragmentShaderName);

		// Make sure `position` is at location 0, since location 0 is required in order for glDraw*() to work.
		glBindAttribLocation(program->glProgramName, 0, "position");

		if (program->glGeometryShaderName)
		{
			GLuint inputPrimitiveGlMode = GL_TRIANGLES;
			if (inputPrimitiveMode == VuoMesh_IndividualLines
			 || inputPrimitiveMode == VuoMesh_LineStrip)
				inputPrimitiveGlMode = GL_LINES;
			else if (inputPrimitiveMode == VuoMesh_Points)
				inputPrimitiveGlMode = GL_POINTS;
			glProgramParameteriEXT(program->glProgramName, GL_GEOMETRY_INPUT_TYPE_EXT, inputPrimitiveGlMode);

			GLuint outputPrimitiveGlMode = GL_TRIANGLE_STRIP;
			if (!program->glFragmentShaderName)
			{
				// If there's no fragment shader, this shader is being used for transform feedback,
				// so the output primitive mode needs to match the input primitive mode.
				if (inputPrimitiveMode == VuoMesh_IndividualLines
				 || inputPrimitiveMode == VuoMesh_LineStrip)
					outputPrimitiveGlMode = GL_LINE_STRIP;
				else if (inputPrimitiveMode == VuoMesh_Points)
					outputPrimitiveGlMode = GL_POINTS;
			}
			glProgramParameteriEXT(program->glProgramName, GL_GEOMETRY_OUTPUT_TYPE_EXT, outputPrimitiveGlMode);

			unsigned int expectedVertexCount = program->expectedOutputPrimitiveCount;
			if (outputPrimitiveGlMode == GL_TRIANGLE_STRIP)
				expectedVertexCount *= 3;
			else if (outputPrimitiveGlMode == GL_LINE_STRIP)
				expectedVertexCount *= 2;
			glProgramParameteriEXT(program->glProgramName, GL_GEOMETRY_VERTICES_OUT_EXT, expectedVertexCount);
		}

		// If there's no fragment shader, this shader is being used for transform feedback.
		if (!program->glFragmentShaderName)
		{
			// We need to use GL_INTERLEAVED_ATTRIBS, since GL_SEPARATE_ATTRIBS is limited to writing to 4 buffers.
			const GLchar *varyings[] = { "outPosition", "outNormal", "outTangent", "outBitangent", "outTextureCoordinate" };
			glTransformFeedbackVaryingsEXT(program->glProgramName, 5, varyings, GL_INTERLEAVED_ATTRIBS_EXT);
		}

		glLinkProgram(program->glProgramName);

		{
			int infologLength = 0;
			int charsWritten  = 0;
			char *infoLog;

			glGetProgramiv(program->glProgramName, GL_INFO_LOG_LENGTH, &infologLength);

			if (infologLength > 0)
			{
				infoLog = (char *)malloc(infologLength);
				glGetProgramInfoLog(program->glProgramName, infologLength, &charsWritten, infoLog);
				VLog("%s: %s", shader->name, infoLog);
				free(infoLog);
			}
		}
	}

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
 * @return `false` if the shader doesn't support the specified `primitiveMode`.
 *
 * @threadAnyGL
 */
bool VuoShader_getAttributeLocations(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext, int *positionLocation, int *normalLocation, int *tangentLocation, int *bitangentLocation, int *textureCoordinateLocation)
{
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
		dispatch_semaphore_signal(shader->lock);
		return false;
	}

	DEFINE_PROGRAM();

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		/// @todo cache values for each program?
		if (positionLocation)
			*positionLocation = glGetAttribLocation(program->glProgramName, "position");
		if (normalLocation)
			*normalLocation = glGetAttribLocation(program->glProgramName, "normal");
		if (tangentLocation)
			*tangentLocation = glGetAttribLocation(program->glProgramName, "tangent");
		if (bitangentLocation)
			*bitangentLocation = glGetAttribLocation(program->glProgramName, "bitangent");
		if (textureCoordinateLocation)
			*textureCoordinateLocation = glGetAttribLocation(program->glProgramName, "textureCoordinate");
	}

	dispatch_semaphore_signal(shader->lock);
	return true;
}

/**
 * Activates the shader program (`glUseProgram()`) on the specified `glContext`,
 * binds the shader's images to texture units, and uploads its unforms,
 * so that the shader is ready for use in rendering.
 *
 * @param shader The shader to activate.
 * @param inputPrimitiveMode The shader program mode to activate.
 * @param glContext The OpenGL context on which to activate the shader program.
 * @return The OpenGL Program Name (which can be used, e.g., to retrieve other uniform locations), or 0 if `inputPrimitiveMode` is invalid or if there is no shader for this `inputPrimitiveMode`.
 *
 * @threadAnyGL
 */
unsigned int VuoShader_activate(VuoShader shader, const VuoMesh_ElementAssemblyMethod inputPrimitiveMode, VuoGlContext glContext)
{
	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER);
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
		VLog("Error: '%s' doesn't have a program for the requested inputPrimitiveMode.", shader->name);
		dispatch_semaphore_signal(shader->lock);
		return 0;
	}

	DEFINE_PROGRAM();

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		glUseProgram(program->glProgramName);

		GLuint textureUnit = 0;
		for (unsigned int i = 0; i < shader->uniformsCount; ++i)
		{
			VuoShaderUniform uniform = shader->uniforms[i];
			GLint location = glGetUniformLocation(program->glProgramName, uniform.name);
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
				VLog("Error: Unknown type %s for '%s'", uniform.type, uniform.name);
		}
	}

	//	dispatch_semaphore_signal(shader->lock); --- hold the lock while the shader is active
	return program->glProgramName;
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
//	dispatch_semaphore_wait(shader->lock, DISPATCH_TIME_FOREVER); --- the lock is held while the shader is active
	if (!VuoShader_ensureUploaded(shader, inputPrimitiveMode, glContext))
	{
//		dispatch_semaphore_signal(shader->lock);
		return;
	}

	DEFINE_PROGRAM();

	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		GLuint textureUnit = 0;
		for (unsigned int i = 0; i < shader->uniformsCount; ++i)
		{
			GLint location = glGetUniformLocation(program->glProgramName, shader->uniforms[i].name);
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

		glUseProgram(0);
	}

	dispatch_semaphore_signal(shader->lock);
	return;
}

/**
 * Decodes the JSON object @c js, expected to contain a 64-bit integer (memory address or 0), to create a new @c VuoShader.
 *
 * @threadAny
 */
VuoShader VuoShader_valueFromJson(json_object *js)
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
json_object * VuoShader_jsonFromValue(const VuoShader value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * Returns a summary of the shader: the text description provided to @ref VuoShader_make.
 *
 * @threadAny
 */
char * VuoShader_summaryFromValue(const VuoShader value)
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
