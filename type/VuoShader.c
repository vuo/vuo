/**
 * @file
 * VuoShader implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"
#include "type.h"
#include "VuoShader.h"
#include "VuoGlContext.h"

/// @todo After we drop 10.6 support, switch back to gl3.h.
//#include <OpenGL/gl3.h>
#include <OpenGL/gl.h>


/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Graphics Shader",
					 "description" : "A graphics shader program, specifying how to render a 3D object.",
					 "keywords" : [ "glsl", "fragment", "vertex" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c",
						 "json",
						 "VuoGlContext",
						 "OpenGL.framework"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoShader
 * Decrements the retain count of the OpenGL Texture Object associated with the specified @c VuoImage,
 * and frees the @c texture VuoImage struct.
 *
 * May be called from any thread (doesn't require an existing GL Context).
 */
void VuoShader_free(void *shader)
{
//	fprintf(stderr, "VuoShader_free(%p)\n", shader);
	VuoShader s = (VuoShader)shader;

	VuoGlContext_use();
	{
		glDetachShader(s->glProgramName, s->glVertexShaderName);
		glDetachShader(s->glProgramName, s->glFragmentShaderName);
		glDeleteShader(s->glVertexShaderName);
		glDeleteShader(s->glFragmentShaderName);
		glDeleteProgram(s->glProgramName);
	}
	VuoGlContext_disuse();

	free(s->summary);
	VuoRelease(s->textures);
	VuoRelease(s->glTextureUniformLocations);

	free(s);
}

/**
 * @ingroup VuoShader
 * Prints GLSL debug information to the console.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoShader_printShaderInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		fprintf(stderr,"%s\n",infoLog);
		free(infoLog);
	}
}

/**
 * @ingroup VuoShader
 * Prints GLSL debug information to the console.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoShader_printProgramInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		fprintf(stderr,"%s\n",infoLog);
		free(infoLog);
	}
}

/**
 * @ingroup VuoShader
 * Compiles the specified shader source.
 *
 * Must be called from a thread with an active GL Context.
 */
static GLuint VuoShader_compileShader(GLenum type, const char * source)
{
	GLint length = strlen(source);
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);
	VuoShader_printShaderInfoLog(shader);
	return shader;
}

/**
 * @ingroup VuoShader
 * Compiles, links, and uploads the specified shader sources.
 *
 * The vertex shader must define:
 *
 *     uniform mat4 projectionMatrix;
 *     uniform mat4 modelviewMatrix;
 *     attribute vec4 position;
 *     attribute vec4 normal;
 *     attribute vec4 tangent;
 *     attribute vec4 bitangent;
 *     attribute vec4 textureCoordinate;
 *
 * May be called from any thread (doesn't require an existing GL Context).
 */
VuoShader VuoShader_make(const char *summary, const char *vertexShaderSource, const char *fragmentShaderSource)
{
//	fprintf(stderr, "VuoShader_make()\n");
	VuoShader t = (VuoShader)malloc(sizeof(struct _VuoShader));
	VuoRegister(t, VuoShader_free);

	t->summary = strdup(summary);

	VuoGlContext_use();
	{
		t->glVertexShaderName = VuoShader_compileShader(GL_VERTEX_SHADER, vertexShaderSource);
		t->glFragmentShaderName = VuoShader_compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

		t->glProgramName = glCreateProgram();
		glAttachShader(t->glProgramName, t->glVertexShaderName);
		glAttachShader(t->glProgramName, t->glFragmentShaderName);
		glLinkProgram(t->glProgramName);
		VuoShader_printProgramInfoLog(t->glProgramName);

		t->textures = VuoListCreate_VuoImage();
		t->glTextureUniformLocations = VuoListCreate_VuoInteger();
	}
	VuoGlContext_disuse();

	VuoRetain(t->textures);
	VuoRetain(t->glTextureUniformLocations);

//	fprintf(stderr, "VuoShader_make() created %p\n", t);
	return t;
}

static const char * defaultVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
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

static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		// Default shader: render an unlit gradient checkerboard
		// (so you can see the object, and get a feel for its texture coordinates).
		// White in the top left corner.
		int toggle = 0;
		toggle += mod(fragmentTextureCoordinate.x*16., 2.) < 1. ? 1 : 0;
		toggle += mod(fragmentTextureCoordinate.y*16., 2.) < 1. ? 1 : 0;
		gl_FragColor = vec4(1.-fragmentTextureCoordinate.x,fragmentTextureCoordinate.y,1.,1.)
			- (toggle == 1 ? vec4(0.,0.,0.,0.) : vec4(.25,.25,.25,0.));
	}
);

/**
 * @ingroup VuoShader
 * Decodes the JSON object @c js, expected to contain a 64-bit integer (memory address or 0), to create a new @c VuoShader.
 */
VuoShader VuoShader_valueFromJson(json_object *js)
{
	VuoShader s = NULL;

	if (json_object_get_type(js) == json_type_int)
		s = (VuoShader) json_object_get_int64(js);

	if (!s)
		return VuoShader_make("default checkerboard shader", defaultVertexShaderSource, checkerboardFragmentShaderSource);

	return s;
}

/**
 * @ingroup VuoShader
 * Encodes @c value as a JSON object.
 *
 * Serializes the pointer to the VuoShader object, since we need to preserve its reference count.
 */
json_object * VuoShader_jsonFromValue(const VuoShader value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * @ingroup VuoShader
 *
 * Returns a summary of the shader: the text description provided to @c VuoShader_make(), and the number of textures associated with the shader.
 */
char * VuoShader_summaryFromValue(const VuoShader value)
{
	if (!value)
		return strdup("(no shader)");

	const char *format = "%s<br>with %d texture%s";
	unsigned long textureCount = VuoListGetCount_VuoImage(value->textures);
	const char *texturePlural = textureCount == 1 ? "" : "s";
	int size = snprintf(NULL, 0, format, value->summary, textureCount, texturePlural);
	char *valueAsString = (char *)malloc(size+1);
	snprintf(valueAsString, size+1, format, value->summary, textureCount, texturePlural);
	return valueAsString;
}

/**
 * Sets a @c float uniform value on the specified @c shader.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
void VuoShader_setUniformFloat(VuoShader shader, const char *uniformIdentifier, float value)
{
	VuoGlContext_use();
	{
		glUseProgram(shader->glProgramName);
		{
			GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
			if (uniform < 0)
				fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
			else
				glUniform1f(uniform, value);
		}
		glUseProgram(0);
	}
	VuoGlContext_disuse();
}

/**
 * Sets a @c float uniform value on the specified @c shader.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
void VuoShader_setUniformPoint2d(VuoShader shader, const char *uniformIdentifier, VuoPoint2d value)
{
	VuoGlContext_use();
	{
		glUseProgram(shader->glProgramName);
		{
			GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
			if (uniform < 0)
				fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
			else
				glUniform2f(uniform, value.x, value.y);
		}
		glUseProgram(0);
	}
	VuoGlContext_disuse();
}

static const char * imageFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		if (color.a < 1./255.)
			discard;
		gl_FragColor = color;
	}
);

/**
 * Returns the default vertex shader, which projects verties and passes through texture coordinates.
 */
const char * VuoShader_getDefaultVertexShader(void)
{
	return defaultVertexShaderSource;
}

/**
 * Converts the provided @a vuoCoordinates into GLSL Sampler Coordinates relative to the provided @a image.
 *
 * Vuo Coordinates range from (-1,-1/aspectRatio) in the bottom left to (1,1/aspectRatio) in the top right.
 *
 * GLSL Sampler Coordinates range from (0,0) to (1,1).
 *
 * If @a image is rendered in a scene, centered at (0,0,0), width 2, at its correct aspect ratio,
 * this function will transform 2D coordinates along the XY plane (at Z=0) into correct sampler coordinates for that image.
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
 */
VuoReal VuoShader_samplerSizeFromVuoSize(VuoReal vuoSize)
{
	return vuoSize/2.;
}

/**
 * Returns a shader that renders objects with an image (ignoring lighting), specified by uniform @c texture.
 */
VuoShader VuoShader_makeImageShader(void)
{
	return VuoShader_make("image shader", defaultVertexShaderSource, imageFragmentShaderSource);
}

/**
 * Empties the list of textures associated with @c shader.
 */
void VuoShader_resetTextures(VuoShader shader)
{
	VuoListRemoveAll_VuoImage(shader->textures);
	VuoListRemoveAll_VuoInteger(shader->glTextureUniformLocations);
}

/**
 * Adds to @c shader an association between @c texture and @c uniformIdentifier.
 *
 * May be called from any thread (doesn't require an existing GL Context).
 */
void VuoShader_addTexture(VuoShader shader, VuoImage texture, const char *uniformIdentifier)
{
	GLint textureUniform;
	VuoGlContext_use();
	{
		textureUniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
	}
	VuoGlContext_disuse();

	if (textureUniform < 0)
	{
		fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
		return;
	}

	VuoListAppendValue_VuoImage(shader->textures, texture);
	VuoListAppendValue_VuoInteger(shader->glTextureUniformLocations, textureUniform);
}

/**
 * Assigns each of the shader's textures to a texture unit, and passes the texture unit number along to the shader.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoShader_activateTextures(VuoShader shader)
{
	unsigned long textureCount = VuoListGetCount_VuoImage(shader->textures);
	for (unsigned int i = 0; i < textureCount; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		VuoImage image = VuoListGetValueAtIndex_VuoImage(shader->textures, i+1);
		glBindTexture(image->glTextureTarget, image->glTextureName);
		glUniform1i(VuoListGetValueAtIndex_VuoInteger(shader->glTextureUniformLocations, i+1), i);
	}
}

/**
 * Unbinds the texture units used by this shader.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoShader_deactivateTextures(VuoShader shader)
{
	unsigned long textureCount = VuoListGetCount_VuoImage(shader->textures);
	for (unsigned int i = 0; i < textureCount; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		VuoImage image = VuoListGetValueAtIndex_VuoImage(shader->textures, i+1);
		glBindTexture(image->glTextureTarget, 0);
	}
}
