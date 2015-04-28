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
#include "VuoGlContext.h"
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
 * @threadAny
 */
void VuoShader_free(void *shader)
{
//	fprintf(stderr, "VuoShader_free(%p)\n", shader);
	VuoShader s = (VuoShader)shader;

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

//		glDetachShader(s->glProgramName, s->glVertexShaderName);
//		glDetachShader(s->glProgramName, s->glFragmentShaderName);
//		glDeleteShader(s->glVertexShaderName);
//		glDeleteShader(s->glFragmentShaderName);
		glDeleteProgram(s->glProgramName);

		VuoGlContext_disuse(cgl_ctx);
	}

	free(s->summary);
	VuoRelease(s->textures);
	VuoRelease(s->glTextureUniformLocations);

	dispatch_release(s->lock);

	free(s);
}

/**
 * @ingroup VuoShader
 * Prints GLSL debug information to the console.
 *
 * @threadAnyGL
 */
void VuoShader_printProgramInfoLog(CGLContextObj cgl_ctx, GLuint obj)
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
 * Compiles, links, and uploads the specified shader sources.
 *
 * @c VuoSceneRenderer automatically provides several uniform values and vertex attributes to shaders.
 * A few are required; the rest are optional.
 *
 *     uniform mat4 projectionMatrix;  // required
 *     uniform mat4 modelviewMatrix;   // required
 *
 *     uniform vec4 cameraPosition;
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
 * @threadAny
 */
VuoShader VuoShader_make(const char *summary, const char *vertexShaderSource, const char *fragmentShaderSource)
{
//	fprintf(stderr, "VuoShader_make()\n");
	VuoShader t = (VuoShader)malloc(sizeof(struct _VuoShader));
	VuoRegister(t, VuoShader_free);

	t->summary = strdup(summary);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		t->glVertexShaderName = VuoGlShader_use(cgl_ctx, GL_VERTEX_SHADER, vertexShaderSource);
		t->glFragmentShaderName = VuoGlShader_use(cgl_ctx, GL_FRAGMENT_SHADER, fragmentShaderSource);

		t->glProgramName = glCreateProgram();
		glAttachShader(t->glProgramName, t->glVertexShaderName);
		glAttachShader(t->glProgramName, t->glFragmentShaderName);
		glLinkProgram(t->glProgramName);
		VuoShader_printProgramInfoLog(cgl_ctx, t->glProgramName);

		t->textures = VuoListCreate_VuoImage();
		t->glTextureUniformLocations = VuoListCreate_VuoInteger();

		// Ensure the command queue gets executed before we return,
		// since the shader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	VuoRetain(t->textures);
	VuoRetain(t->glTextureUniformLocations);

	t->lock = dispatch_semaphore_create(1);

//	fprintf(stderr, "VuoShader_make() created %p\n", t);
	return t;
}

/**
 * A basic linear-projection vertex shader.
 */
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

/**
 * Default shader: render an unlit gradient checkerboard
 * (so you can see the object, and get a feel for its texture coordinates).
 * White in the top left corner.
 */
static const char * checkerboardFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
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
 *
 * @threadAny
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
 *
 * @threadAny
 */
json_object * VuoShader_jsonFromValue(const VuoShader value)
{
	return json_object_new_int64((int64_t)value);
}

/**
 * @ingroup VuoShader
 *
 * Returns a summary of the shader: the text description provided to @c VuoShader_make(), and the number of textures associated with the shader.
 *
 * @threadAny
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
 * @threadAnyGL
 */
void VuoShader_setUniformFloat(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, float value)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

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

/**
 * Sets a @c float[] uniform value on the specified @c shader.
 *
 * @threadAnyGL
 */
void VuoShader_setUniformFloatArray(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, const float* value, int length)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glUseProgram(shader->glProgramName);
	{
		GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
		if (uniform < 0)
			fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
		else
			glUniform1fv(uniform, length, value);
	}
	glUseProgram(0);
}

/**
 * Sets a @c vec2 uniform value on the specified @c shader.
 *
 * @threadAnyGL
 */
void VuoShader_setUniformPoint2d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint2d value)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

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

/**
 * Sets a @c vec3 uniform value on the specified @c shader.
 *
 * @threadAnyGL
 */
void VuoShader_setUniformPoint3d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint3d value)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glUseProgram(shader->glProgramName);
	{
		GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
		if (uniform < 0)
			fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
		else
			glUniform3f(uniform, value.x , value.y, value.z);
	}
	glUseProgram(0);
}

/**
 * Sets a @c vec4 uniform value on the specified @c shader.
 *
 * @threadAnyGL
 */
void VuoShader_setUniformPoint4d(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoPoint4d value)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glUseProgram(shader->glProgramName);
	{
		GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
		if (uniform < 0)
			fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
		else
			glUniform4f(uniform, value.x , value.y, value.z, value.w);
	}
	glUseProgram(0);
}

/**
 * Sets a @c vec4 uniform value on the specified @c shader accepting a VuoColor.
 *
 * @threadAnyGL
 */
void VuoShader_setUniformColor(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoColor value)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glUseProgram(shader->glProgramName);
	{
		GLint uniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
		if (uniform < 0)
			fprintf(stderr, "Error: Couldn't find uniform '%s' in shader '%s'.\n", uniformIdentifier, shader->summary);
		else
			glUniform4f(uniform, value.r , value.g, value.b, value.a);
	}
	glUseProgram(0);
}

/**
 * Renders an unlit (full-intensity) image.
 */
static const char * imageFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float alpha;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
		color.rgb /= color.a;	// un-premultiply
		color.a *= alpha;
		if (color.a < 1./255.)
			discard;
		gl_FragColor = color;
	}
);

/**
 * Returns the default vertex shader, which projects verties and passes through texture coordinates.
 *
 * @threadAny
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
 * Returns a shader that renders objects with an image (ignoring lighting), specified by uniform @c texture.
 *
 * @threadAny
 */
VuoShader VuoShader_makeImageShader(void)
{
	return VuoShader_make("image shader", defaultVertexShaderSource, imageFragmentShaderSource);
}

/**
 * Empties the list of textures associated with @c shader.
 *
 * @threadAny
 */
void VuoShader_resetTextures(VuoShader shader)
{
	VuoListRemoveAll_VuoImage(shader->textures);
	VuoListRemoveAll_VuoInteger(shader->glTextureUniformLocations);
}

/**
 * Adds to @c shader an association between @c texture and @c uniformIdentifier.
 *
 * @threadAnyGL
 */
void VuoShader_addTexture(VuoShader shader, VuoGlContext glContext, const char *uniformIdentifier, VuoImage texture)
{
	GLint textureUniform;
	{
		CGLContextObj cgl_ctx = (CGLContextObj)glContext;

		/// @todo instead of querying this every time an image is added, query once when the shader is compiled, and store it in a dictionary in VuoShader.
		textureUniform = glGetUniformLocation(shader->glProgramName, uniformIdentifier);
	}

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
 * @threadAnyGL
 */
void VuoShader_activateTextures(VuoShader shader, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

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
 * @threadAnyGL
 */
void VuoShader_deactivateTextures(VuoShader shader, VuoGlContext glContext)
{
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	unsigned long textureCount = VuoListGetCount_VuoImage(shader->textures);
	for (unsigned int i = 0; i < textureCount; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		VuoImage image = VuoListGetValueAtIndex_VuoImage(shader->textures, i+1);
		glBindTexture(image->glTextureTarget, 0);
	}
}

/**
 * Returns a shader that renders a solid @c color.
 */
VuoShader VuoShader_makeColorShader(VuoColor color)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform mat4 projectionMatrix;
		uniform mat4 modelviewMatrix;
		attribute vec4 position;

		void main()
		{
			gl_Position = projectionMatrix * modelviewMatrix * position;
		}
	);

	const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform vec4 color;

		void main()
		{
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("solid color shader", vertexShaderSource, fragmentShaderSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		glUseProgram(shader->glProgramName);
		{
			GLint colorUniform = glGetUniformLocation(shader->glProgramName, "color");
			glUniform4f(colorUniform, color.r, color.g, color.b, color.a);
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}

/// Defines the uniforms passed by VuoSceneRenderer, the varyings provided by @c lightingVertexShaderSource, and the @c calculateLighting() function.
#define VUOSHADER_GLSL_FRAGMENT_LIGHTING_HEADER "																										 	\
	uniform mat4 modelviewMatrix;																														 	\
																																							\
	uniform vec3 cameraPosition;																														 	\
																																							\
	uniform vec4 ambientColor;																															 	\
	uniform float ambientBrightness;																													 	\
																																							\
	struct PointLight																																	 	\
	{																																					 	\
		vec4 color;																																		 	\
		float brightness;																																 	\
		vec3 position;																																	 	\
		float range;																																	 	\
		float sharpness;																																 	\
	};																																					 	\
	uniform PointLight pointLights[16];																													 	\
	uniform int pointLightCount;																														 	\
																																							\
	struct SpotLight																																	 	\
	{																																					 	\
		vec4 color;																																		 	\
		float brightness;																																 	\
		vec3 position;																																	 	\
		vec3 direction;																																	 	\
		float cone;																																		 	\
		float range;																																	 	\
		float sharpness;																																 	\
	};																																					 	\
	uniform SpotLight spotLights[16];																													 	\
	uniform int spotLightCount;																															 	\
																																							\
	varying vec4 vertexPosition;																															\
	varying mat3 vertexPlaneToWorld;																														\
																																							\
	void calculateLighting(																																	\
			in float specularPower,																															\
			in vec3 normal,																																	\
			out vec3 ambientContribution,																													\
			out vec3 diffuseContribution,																													\
			out vec3 specularContribution																													\
		)																																					\
	{																																					 	\
		ambientContribution = ambientColor.rgb * ambientColor.a * ambientBrightness;																	 	\
		diffuseContribution = specularContribution = vec3(0.);																							 	\
																																							\
		vec3 normalDirection = normalize(vertexPlaneToWorld * normal);																						\
																																							\
		int i;																																			 	\
		for (i=0; i<pointLightCount; ++i)																												 	\
		{																																				 	\
			float lightDistance = distance(pointLights[i].position, vertexPosition.xyz);																 	\
			float range = pointLights[i].range;																											 	\
			float sharpness = pointLights[i].sharpness;																									 	\
			float lightRangeFactor = 1. - smoothstep(range*sharpness, range*(2-sharpness), lightDistance);												 	\
																																							\
			vec3 scaledLightColor = lightRangeFactor * pointLights[i].color.rgb * pointLights[i].color.a * pointLights[i].brightness;					 	\
																																							\
			vec3 incidentLightDirection = normalize(pointLights[i].position - vertexPosition.xyz);														 	\
			diffuseContribution += scaledLightColor * max(dot(normalDirection, incidentLightDirection), 0.);											 	\
																																							\
			vec3 reflection = reflect(-incidentLightDirection, normalDirection);																		 	\
			vec3 cameraDirection = normalize(cameraPosition.xyz - vertexPosition.xyz);																	 	\
			specularContribution += scaledLightColor * pow(max(dot(reflection, cameraDirection), 0.), specularPower);									 	\
		}																																				 	\
																																							\
		for (i=0; i<spotLightCount; ++i)																												 	\
		{																																				 	\
			float lightDistance = distance(spotLights[i].position, vertexPosition.xyz);																	 	\
			float range = spotLights[i].range;																											 	\
			float sharpness = spotLights[i].sharpness;																										\
			float lightRangeFactor = 1. - smoothstep(range*sharpness, range*(2-sharpness), lightDistance);													\
																																							\
			vec3 incidentLightDirection = normalize(spotLights[i].position - vertexPosition.xyz);														 	\
			float cosSpotDirection = dot(-incidentLightDirection, spotLights[i].direction);																 	\
			float cone = spotLights[i].cone;																											 	\
			float lightDirectionFactor = smoothstep(cos(cone*(2-sharpness)/2), cos(cone*sharpness/2), cosSpotDirection);									\
																																							\
			vec3 scaledLightColor = lightRangeFactor * lightDirectionFactor * spotLights[i].color.rgb * spotLights[i].color.a * spotLights[i].brightness;	\
																																							\
			diffuseContribution += scaledLightColor * max(dot(normalDirection, incidentLightDirection), 0.);											 	\
																																							\
			vec3 reflection = reflect(-incidentLightDirection, normalDirection);																		 	\
			vec3 cameraDirection = normalize(cameraPosition.xyz - vertexPosition.xyz);																	 	\
			specularContribution += scaledLightColor * pow(max(dot(reflection, cameraDirection), 0.), specularPower);									 	\
		}																																				 	\
	}																																					 	\
"

/// Quotes @c source into a C string constant, prefixed with @c VUOSHADER_GLSL_FRAGMENT_LIGHTING_HEADER.
#define VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_LIGHTING(source) "#version 120\n" VUOSHADER_GLSL_FRAGMENT_LIGHTING_HEADER "\n" #source

/**
 * Returns a shader that renders a color with lighting.
 *
 * @param diffuseColor The primary material color.
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 */
VuoShader VuoShader_makeLitColorShader(VuoColor diffuseColor, VuoColor highlightColor, VuoReal shininess)
{
	const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs provided by VuoSceneRenderer
		uniform mat4 projectionMatrix;
		uniform mat4 modelviewMatrix;
		attribute vec4 position;
		attribute vec4 normal;
		attribute vec4 tangent;
		attribute vec4 bitangent;

		// Outputs to fragment shader
		varying vec4 vertexPosition;
		varying mat3 vertexPlaneToWorld;

		void main()
		{
			vertexPosition = modelviewMatrix * position;

			vertexPlaneToWorld[0] = normalize(vec3(modelviewMatrix * vec4(tangent.xyz,0.)));
			vertexPlaneToWorld[1] = normalize(vec3(modelviewMatrix * -vec4(bitangent.xyz,0.)));
			vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix * vec4(normal.xyz,0.)));

			gl_Position = projectionMatrix * vertexPosition;
		}
	);

	const char *fragmentShaderSource = VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_LIGHTING(
		// Inputs from ports
		uniform vec4 diffuseColor;
		uniform vec4 specularColor;
		uniform float specularPower;

		void main()
		{
			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, vec3(0,0,1), ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= diffuseColor.rgb;
			diffuseContribution *= diffuseColor.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, diffuseColor.a);
		}
	);

	VuoShader shader = VuoShader_make("lit color shader", vertexShaderSource, fragmentShaderSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		glUseProgram(shader->glProgramName);
		{
			GLint colorUniform = glGetUniformLocation(shader->glProgramName, "diffuseColor");
			glUniform4f(colorUniform, diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

			GLint highlightColorUniform = glGetUniformLocation(shader->glProgramName, "specularColor");
			glUniform4f(highlightColorUniform, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);

			GLint shininessUniform = glGetUniformLocation(shader->glProgramName, "specularPower");
			glUniform1f(shininessUniform, 1./(1.0001-shininess));
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}

/**
 * A linear-projection vertex shader.
 * Also builds a matrix that transforms between world coordinates and coordinates on a plane tangent to the surface.
 */
static const char *lightingVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	// Outputs to fragment shader
	varying vec4 vertexPosition;
	varying vec4 fragmentTextureCoordinate;
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		vertexPosition = modelviewMatrix * position;

		fragmentTextureCoordinate = textureCoordinate;

		vertexPlaneToWorld[0] = normalize(vec3(modelviewMatrix * vec4(tangent.xyz,0.)));
		vertexPlaneToWorld[1] = normalize(vec3(modelviewMatrix * -vec4(bitangent.xyz,0.)));
		vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix * vec4(normal.xyz,0.)));

		gl_Position = projectionMatrix * vertexPosition;
	}
);

/**
 * Returns a shader that renders an image with lighting.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param highlightColor The color of shiny specular highlights. Alpha controls the intensity of the highlights.
 * @param shininess A number representing how shiny the material is.  0 = dull; 1 = shiny; numbers in between represent varying amounts of shininess.
 */
VuoShader VuoShader_makeLitImageShader(VuoImage image, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_LIGHTING(
		// Inputs from vertex shader
		varying vec4 fragmentTextureCoordinate;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform vec4 specularColor;
		uniform float specularPower;

		void main()
		{
			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			calculateLighting(specularPower, vec3(0,0,1), ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb;
			diffuseContribution *= color.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a);
		}
	);

	VuoShader shader = VuoShader_make("lit image shader", lightingVertexShaderSource, fragmentShaderSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		VuoShader_addTexture(shader, cgl_ctx, "texture", image);

		glUseProgram(shader->glProgramName);
		{
			GLint alphaUniform = glGetUniformLocation(shader->glProgramName, "alpha");
			glUniform1f(alphaUniform, alpha);

			GLint highlightColorUniform = glGetUniformLocation(shader->glProgramName, "specularColor");
			glUniform4f(highlightColorUniform, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);

			GLint shininessUniform = glGetUniformLocation(shader->glProgramName, "specularPower");
			glUniform1f(shininessUniform, 1./(1.0001-shininess));
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}

/**
 * Returns a shader that renders an image with lighting and surface details.
 *
 * @param image The image which provides the diffuse / primary material color.
 * @param alpha The opacity of the image (0 to 1).
 * @param specularImage An image that specifies the specular color (RGB) and shininess (A).
 * @param normalImage An image that specifies the surface details.  The red and green channels respectively define the normal direction along the tangent and bitangent axes (0 = negative; 0.5 = straight; 1 = positive).  The blue channel defines the height along the normal axis (0 = low; 1 = high).
 */
VuoShader VuoShader_makeLitImageDetailsShader(VuoImage image, VuoReal alpha, VuoImage specularImage, VuoImage normalImage)
{
	if (!image || !specularImage || !normalImage)
		return NULL;

	const char *fragmentShaderSource = VUOSHADER_GLSL_FRAGMENT_SOURCE_WITH_LIGHTING(
		// Inputs from vertex shader
		varying vec4 fragmentTextureCoordinate;

		// Inputs from ports
		uniform sampler2D texture;
		uniform float alpha;
		uniform sampler2D specularImage;
		uniform sampler2D normalImage;

		void main()
		{
			vec4 color = texture2D(texture, fragmentTextureCoordinate.xy);
			color.rgb /= color.a;	// un-premultiply
			color.a *= alpha;
			if (color.a < 1./255.)
				discard;

			vec3 ambientContribution = vec3(0.);
			vec3 diffuseContribution = vec3(0.);
			vec3 specularContribution = vec3(0.);

			vec4 specularColor = texture2D(specularImage, fragmentTextureCoordinate.xy);
			float specularPower = 1./(1.0001-specularColor.a);

			vec4 normalColor = texture2D(normalImage, fragmentTextureCoordinate.xy);
			vec3 normal = normalize(vec3(
						2. * normalColor.r - 1.,
						2. * normalColor.g - 1.,
						normalColor.b	// Leave the blue channel as-is; the normal should never point inward.
						));

			calculateLighting(specularPower, normal, ambientContribution, diffuseContribution, specularContribution);

			ambientContribution *= color.rgb;
			diffuseContribution *= color.rgb;
			specularContribution *= specularColor.rgb * specularColor.a;
			gl_FragColor = vec4(ambientContribution + diffuseContribution + specularContribution, color.a);
		}
	);

	VuoShader shader = VuoShader_make("lit image details shader", lightingVertexShaderSource, fragmentShaderSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		VuoShader_addTexture(shader, cgl_ctx, "texture", image);
		VuoShader_addTexture(shader, cgl_ctx, "specularImage", specularImage);
		VuoShader_addTexture(shader, cgl_ctx, "normalImage", normalImage);

		glUseProgram(shader->glProgramName);
		{
			GLint alphaUniform = glGetUniformLocation(shader->glProgramName, "alpha");
			glUniform1f(alphaUniform, alpha);
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}

/**
 *	Returns a shader that renders a linear gradient using the provided colors and start and end coordinates.
 *	Coordinates should be passed in Vuo scene coordinates (-1,-1) to (1,1).
 */
VuoShader VuoShader_makeLinearGradientShader(VuoList_VuoColor colors, VuoPoint2d start, VuoPoint2d end)
{
	const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,

		uniform float gradientCount;
		uniform sampler2D gradientStrip;
		uniform vec2 start;
		uniform vec2 end;
		varying vec4 fragmentTextureCoordinate;

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
			vec2 pol = nearest_point_on_line(start, end, fragmentTextureCoordinate.xy);
			float x = dot(pol-start, end-start) > 0 ? distance(start, pol)/ distance(start, end) : 0;
			float gradientWidth = (1./gradientCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;	// scale to account for the gradient/2 offsets
			vec4 color = texture2D(gradientStrip, vec2(clamp(x , gradientWidth, 1-gradientWidth), .5));
			color.rgb /= color.a;	// un-premultiply
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Linear Gradient Shader", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
	VuoShader_resetTextures(shader);

	int len = VuoListGetCount_VuoColor(colors);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValueAtIndex_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_RGBA, len, 1);
	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		GLint textureUniform = glGetUniformLocation(shader->glProgramName, "gradientStrip");

		if(textureUniform < 0)
			VLog("textureUniform not found");

		VuoListAppendValue_VuoImage(shader->textures, gradientStrip);
		VuoListAppendValue_VuoInteger(shader->glTextureUniformLocations, textureUniform);

		glUseProgram(shader->glProgramName);
		{
			// Error checking shouldn't be necessary since the user doesn't get to set this, right?
			GLint uniform = glGetUniformLocation(shader->glProgramName, "gradientCount");
			glUniform1f(uniform, (float)len);
			uniform = glGetUniformLocation(shader->glProgramName, "start");
			glUniform2f(uniform, (start.x+1)/2, (start.y+1)/2);
			uniform = glGetUniformLocation(shader->glProgramName, "end");
			glUniform2f(uniform, (end.x+1)/2, (end.y+1)/2);
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}

/**
 *	Returns a shader that renders a radial gradient using the provided colors, center point, and radius.
 *	Center and radius are expected in Vuo scene coordinates.  Width and Height may be either pixels or scene coordinates, as they
 *	are only used to calculate the aspect ratio.
 */
VuoShader VuoShader_makeRadialGradientShader(VuoList_VuoColor colors, VuoPoint2d center, VuoReal radius, VuoReal width, VuoReal height)
{
	const char * radialGradientFragSource = VUOSHADER_GLSL_SOURCE(120,
		uniform float gradientCount;
		uniform sampler2D gradientStrip;
		uniform vec2 center;
		uniform vec2 scale;	// if image is not square, multiply texCoord by this to account for stretch
		uniform float radius;
		varying vec4 fragmentTextureCoordinate;

		void main(void)
		{
			vec2 scaledTexCoord = fragmentTextureCoordinate.xy*scale;
			float x = distance(center*scale, scaledTexCoord)/radius;
			float gradientWidth = (1./gradientCount)/2.;
			x = x * (1-gradientWidth*2) + gradientWidth;
			vec4 color = texture2D(gradientStrip, vec2(clamp(x , gradientWidth, 1-gradientWidth), .5));
			color.rgb /= color.a;	// un-premultiply
			gl_FragColor = color;
		}
	);

	VuoShader shader = VuoShader_make("Radial Gradient Shader", VuoShader_getDefaultVertexShader(), radialGradientFragSource);
	VuoShader_resetTextures(shader);

	// VuoPoint2d scale = width < height ? VuoPoint2d_make(1., height/(float)width) : VuoPoint2d_make(width/(float)height, 1.);
	VuoPoint2d scale = VuoPoint2d_make(1., height/(float)width);

	// todo Is this used enough to warrant it's own function?
	int len = VuoListGetCount_VuoColor(colors);
	unsigned char* pixels = (unsigned char*)malloc(sizeof(char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValueAtIndex_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}
	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_RGBA, len, 1);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		GLint textureUniform = glGetUniformLocation(shader->glProgramName, "gradientStrip");

		if(textureUniform < 0)
			VLog("textureUniform not found");

		VuoListAppendValue_VuoImage(shader->textures, gradientStrip);
		VuoListAppendValue_VuoInteger(shader->glTextureUniformLocations, textureUniform);

		glUseProgram(shader->glProgramName);
		{
			GLint uniform = glGetUniformLocation(shader->glProgramName, "gradientCount");
			glUniform1f(uniform, (float)len);

			uniform = glGetUniformLocation(shader->glProgramName, "center");
			glUniform2f(uniform, (center.x+1)/2, (center.y+1)/2);

			uniform = glGetUniformLocation(shader->glProgramName, "radius");
			glUniform1f(uniform, radius/2);

			uniform = glGetUniformLocation(shader->glProgramName, "scale");
			glUniform2f(uniform, scale.x, scale.y);
		}
		glUseProgram(0);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}

	return shader;
}
