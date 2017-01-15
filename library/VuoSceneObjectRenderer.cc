/**
 * @file
 * VuoSceneObjectRenderer implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"
#include "VuoSceneRenderer.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <stdlib.h>
#include <list>
#include <string>

#include <IOSurface/IOSurface.h>
#include <CoreServices/CoreServices.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSceneObjectRenderer",
					 "dependencies" : [
						 "VuoSceneObject",
						 "VuoSceneRenderer",
						 "VuoShader",
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ]
				 });
#endif
}

/**
 * OpenGL attribute locations.
 */
typedef struct
{
	GLint position;
	GLint normal;
	GLint tangent;
	GLint bitangent;
	GLint textureCoordinate;

	unsigned int expectedOutputPrimitiveCount;
	bool mayChangeOutputPrimitiveCount;
} VuoSceneObjectRenderer_Attributes;

/**
 * Internal state data for a VuoSceneObjectRenderer instance.
 */
struct VuoSceneObjectRendererInternal
{
	VuoGlContext glContext;

	VuoShader shader;

	GLuint shamTexture;
	GLuint shamFramebuffer;

	GLuint query;

	GLuint vertexArray;

	VuoSceneObjectRenderer_Attributes pointAttributes;
	VuoSceneObjectRenderer_Attributes lineAttributes;
	VuoSceneObjectRenderer_Attributes triangleAttributes;
};

void VuoSceneObjectRenderer_destroy(VuoSceneObjectRenderer sor);

/**
 * Creates a reference-counted object for applying a shader to a @ref VuoSceneObject.
 *
 * @threadAny
 */
VuoSceneObjectRenderer VuoSceneObjectRenderer_make(VuoGlContext glContext, VuoShader shader)
{
	if (!VuoShader_isTransformFeedback(shader))
	{
		VUserLog("Error '%s' is not a transform feedback shader.", shader->name);
		return NULL;
	}

	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)malloc(sizeof(struct VuoSceneObjectRendererInternal));

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	sceneObjectRenderer->glContext = glContext;

	// Fetch the shader's attribute locations
	bool havePoints    = VuoShader_getAttributeLocations(shader, VuoMesh_Points,              cgl_ctx, &sceneObjectRenderer->pointAttributes.position,    &sceneObjectRenderer->pointAttributes.normal,    &sceneObjectRenderer->pointAttributes.tangent,    &sceneObjectRenderer->pointAttributes.bitangent,    &sceneObjectRenderer->pointAttributes.textureCoordinate   );
	bool haveLines     = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualLines,     cgl_ctx, &sceneObjectRenderer->lineAttributes.position,     &sceneObjectRenderer->lineAttributes.normal,     &sceneObjectRenderer->lineAttributes.tangent,     &sceneObjectRenderer->lineAttributes.bitangent,     &sceneObjectRenderer->lineAttributes.textureCoordinate    );
	bool haveTriangles = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &sceneObjectRenderer->triangleAttributes.position, &sceneObjectRenderer->triangleAttributes.normal, &sceneObjectRenderer->triangleAttributes.tangent, &sceneObjectRenderer->triangleAttributes.bitangent, &sceneObjectRenderer->triangleAttributes.textureCoordinate);
	if (!havePoints || !haveLines || !haveTriangles)
	{
		VUserLog("Error: '%s' is missing programs for: %s %s %s", shader->name, havePoints? "" : "points", haveLines? "" : "lines", haveTriangles? "" : "triangles");
		free(sceneObjectRenderer);
		return NULL;
	}

	sceneObjectRenderer->pointAttributes.expectedOutputPrimitiveCount     = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_Points);
	sceneObjectRenderer->pointAttributes.mayChangeOutputPrimitiveCount    = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_Points);

	sceneObjectRenderer->lineAttributes.expectedOutputPrimitiveCount      = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_IndividualLines);
	sceneObjectRenderer->lineAttributes.mayChangeOutputPrimitiveCount     = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_IndividualLines);

	sceneObjectRenderer->triangleAttributes.expectedOutputPrimitiveCount  = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_IndividualTriangles);
	sceneObjectRenderer->triangleAttributes.mayChangeOutputPrimitiveCount = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_IndividualTriangles);

	VuoRegister(sceneObjectRenderer, VuoSceneObjectRenderer_destroy);

	sceneObjectRenderer->shader = shader;
	VuoRetain(sceneObjectRenderer->shader);

	glGenVertexArrays(1, &sceneObjectRenderer->vertexArray);
	glBindVertexArray(sceneObjectRenderer->vertexArray);

	// http://stackoverflow.com/questions/24112671/transform-feedback-without-a-framebuffer
	sceneObjectRenderer->shamTexture = VuoGlTexturePool_use(cgl_ctx, GL_RGBA, 1, 1, GL_BGRA);
	VuoGlTexture_retain(sceneObjectRenderer->shamTexture, NULL, NULL);
	glGenFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneObjectRenderer->shamFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneObjectRenderer->shamTexture, 0);

	glGenQueries(1, &sceneObjectRenderer->query);

	glEnable(GL_RASTERIZER_DISCARD_EXT);

	return (VuoSceneObjectRenderer)sceneObjectRenderer;
}

/**
 * Helper for @ref VuoSceneObjectRenderer_draw.
 * Applies a shader to a single @c sceneObject's VuoMesh (ignoring its childObjects).
 */
static void VuoSceneObjectRenderer_drawSingle(CGLContextObj cgl_ctx, struct VuoSceneObjectRendererInternal *sceneObjectRenderer, VuoSceneObject *sceneObject, float modelviewMatrix[16])
{
	if (!sceneObject->mesh)
		return;

	dispatch_semaphore_wait(VuoSceneRenderer_vertexArraySemaphore, DISPATCH_TIME_FOREVER);

	VuoRetain(sceneObject->mesh);

	VuoMesh newMesh = VuoMesh_make(sceneObject->mesh->submeshCount);

	for (unsigned int i = 0; i < sceneObject->mesh->submeshCount; ++i)
	{
		VuoSubmesh submesh = sceneObject->mesh->submeshes[i];


		VuoMesh_ElementAssemblyMethod outputPrimitiveMode = VuoMesh_getExpandedPrimitiveMode(submesh.elementAssemblyMethod);
		GLuint outputPrimitiveGlMode;
		VuoSceneObjectRenderer_Attributes attributes;
		int primitiveVertexMultiplier;
		if (outputPrimitiveMode == VuoMesh_IndividualTriangles)
		{
			outputPrimitiveGlMode = GL_TRIANGLES;
			attributes = sceneObjectRenderer->triangleAttributes;
			primitiveVertexMultiplier = 3;
		}
		else if (outputPrimitiveMode == VuoMesh_IndividualLines)
		{
			outputPrimitiveGlMode = GL_LINES;
			attributes = sceneObjectRenderer->lineAttributes;
			primitiveVertexMultiplier = 2;
		}
		else // if (submesh.elementAssemblyMethod == VuoMesh_Points)
		{
			outputPrimitiveGlMode = GL_POINTS;
			attributes = sceneObjectRenderer->pointAttributes;
			primitiveVertexMultiplier = 1;
		}


		// Attach the input combinedBuffer for rendering.
		glBindBuffer(GL_ARRAY_BUFFER, submesh.glUpload.combinedBuffer);

		int bufferCount = 0;
		++bufferCount; // position
		if (submesh.glUpload.normalOffset)
			++bufferCount;
		if (submesh.glUpload.tangentOffset)
			++bufferCount;
		if (submesh.glUpload.bitangentOffset)
			++bufferCount;
		if (submesh.glUpload.textureCoordinateOffset)
			++bufferCount;


		VuoGlProgram program;
		if (!VuoShader_activate(sceneObjectRenderer->shader, submesh.elementAssemblyMethod, sceneObjectRenderer->glContext, &program))
		{
			VUserLog("Shader activation failed.");
			return;
		}


		GLint modelviewMatrixUniform = VuoGlProgram_getUniformLocation(program, "modelviewMatrix");
		if (modelviewMatrixUniform != -1)
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);


		GLint modelviewMatrixInverseUniform = VuoGlProgram_getUniformLocation(program, "modelviewMatrixInverse");
		if (modelviewMatrixInverseUniform != -1)
		{
			float modelviewMatrixInverse[16];
			VuoTransform_invertMatrix4x4(modelviewMatrix, modelviewMatrixInverse);
			glUniformMatrix4fv(modelviewMatrixInverseUniform, 1, GL_FALSE, modelviewMatrixInverse);
		}


		glEnableVertexAttribArray(attributes.position);
		glVertexAttribPointer(attributes.position, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, (void*)0);
		if (submesh.glUpload.normalOffset && attributes.normal >= 0)
		{
			glEnableVertexAttribArray(attributes.normal);
			glVertexAttribPointer(attributes.normal, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.normalOffset);
		}
		if (submesh.glUpload.tangentOffset && attributes.tangent >= 0)
		{
			glEnableVertexAttribArray(attributes.tangent);
			glVertexAttribPointer(attributes.tangent, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.tangentOffset);
		}
		if (submesh.glUpload.bitangentOffset && attributes.bitangent >= 0)
		{
			glEnableVertexAttribArray(attributes.bitangent);
			glVertexAttribPointer(attributes.bitangent, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.bitangentOffset);
		}
		if (submesh.glUpload.textureCoordinateOffset && attributes.textureCoordinate >= 0)
		{
			glEnableVertexAttribArray(attributes.textureCoordinate);
			glVertexAttribPointer(attributes.textureCoordinate, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.textureCoordinateOffset);
		}


		// Attach the input elementBuffer for rendering.
		if (submesh.glUpload.elementBuffer)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.glUpload.elementBuffer);


		// Create and attach the output combinedBuffer.
		// The output buffer will always contain all 5 vertex attributes (position, normal, tangent, bitangent, textureCoordinate).
		unsigned long outputVertexCount = VuoSubmesh_getSplitVertexCount(submesh) * attributes.expectedOutputPrimitiveCount;
		unsigned long singleOutputBufferSize = sizeof(VuoPoint4d)*outputVertexCount;
		unsigned long combinedOutputBufferSize = singleOutputBufferSize*5;
		GLuint combinedOutputBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer, combinedOutputBufferSize);
		VuoGlPool_retain(combinedOutputBuffer);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBuffer);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBufferSize, NULL, GL_STATIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, combinedOutputBuffer);


		// Execute the filter.
		GLenum mode = VuoSubmesh_getGlMode(submesh);
		if (attributes.mayChangeOutputPrimitiveCount)
			glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT, sceneObjectRenderer->query);
		glBeginTransformFeedbackEXT(outputPrimitiveGlMode);

#ifdef PROFILE
	GLuint timeElapsedQuery;
	double timeStart = 0;
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
	if (macMinorVersion < 9)
	{
		// Prior to OS X v10.9, glGetQueryObjectuiv() isn't likely to work.
		// (On NVIDIA GeForce 9400M on OS X v10.8, it hangs for 6 seconds then returns bogus data.)
		// https://www.mail-archive.com/mac-opengl@lists.apple.com/msg00003.html
		// https://b33p.net/kosada/node/10677
		glFinish();
		timeStart = VuoLogGetTime();
	}
	else
	{
		glGenQueries(1, &timeElapsedQuery);
		glBeginQuery(GL_TIME_ELAPSED_EXT, timeElapsedQuery);
	}
#endif

		if (submesh.elementCount)
			glDrawElements(mode, submesh.elementCount, GL_UNSIGNED_INT, (void*)0);
		else
			glDrawArrays(mode, 0, submesh.vertexCount);

#ifdef PROFILE
	double seconds;
	if (macMinorVersion < 9)
	{
		glFinish();
		seconds = VuoLogGetTime() - timeStart;
	}
	else
	{
		glEndQuery(GL_TIME_ELAPSED_EXT);
		GLuint nanoseconds;
		glGetQueryObjectuiv(timeElapsedQuery, GL_QUERY_RESULT, &nanoseconds);
		seconds = ((double)nanoseconds) / NSEC_PER_SEC;
		glDeleteQueries(1, &timeElapsedQuery);
	}

	double objectPercent = seconds / (1./60.) * 100.;
	VLog("%6.2f %% of 60 Hz frame	%s (%s)", objectPercent, sceneObject->name, sceneObjectRenderer->shader->name);
#endif


		glEndTransformFeedbackEXT();
		if (attributes.mayChangeOutputPrimitiveCount)
			glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0);

		if (submesh.glUpload.elementBuffer)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (submesh.glUpload.textureCoordinateOffset && attributes.textureCoordinate >= 0)
			glDisableVertexAttribArray(attributes.textureCoordinate);
		if (submesh.glUpload.bitangentOffset && attributes.bitangent >= 0)
			glDisableVertexAttribArray(attributes.bitangent);
		if (submesh.glUpload.tangentOffset && attributes.tangent >= 0)
			glDisableVertexAttribArray(attributes.tangent);
		if (submesh.glUpload.normalOffset && attributes.normal >= 0)
			glDisableVertexAttribArray(attributes.normal);
		glDisableVertexAttribArray(attributes.position);

		VuoShader_deactivate(sceneObjectRenderer->shader, submesh.elementAssemblyMethod, sceneObjectRenderer->glContext);


		GLuint actualVertexCount = 0;
		if (attributes.mayChangeOutputPrimitiveCount)
		{
			glGetQueryObjectuiv(sceneObjectRenderer->query, GL_QUERY_RESULT, &actualVertexCount);
			actualVertexCount *= primitiveVertexMultiplier;
		}
		else
			actualVertexCount = outputVertexCount;


		// Print out the result of the filter, for debugging.
//		glFlush();
//		GLfloat feedback[actualVertexCount*4*5];
//		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, sizeof(feedback), feedback);
//		for (int vertex = 0; vertex < actualVertexCount; vertex++)
//		{
//			for (int coordinate = 0; coordinate < 3; ++coordinate)
//				VLog("\t%f", feedback[vertex*4*bufferCount + coordinate]);
//			VL();
//		}


		newMesh->submeshes[i] = VuoSubmesh_makeGl(
					actualVertexCount, combinedOutputBuffer, combinedOutputBufferSize,
					(void*)(sizeof(VuoPoint4d)*1),
					(void*)(sizeof(VuoPoint4d)*2),
					(void*)(sizeof(VuoPoint4d)*3),
					(void*)(sizeof(VuoPoint4d)*4),
					0, 0, 0, // Since transform feedback doesn't provide an elementBuffer, render this submesh using glDrawArrays().
					outputPrimitiveMode);
		newMesh->submeshes[i].faceCullingMode = submesh.faceCullingMode;
		newMesh->submeshes[i].primitiveSize = submesh.primitiveSize;
	}

//	VuoRetain(newMesh);
	VuoRelease(sceneObject->mesh);
	sceneObject->mesh = newMesh;

	dispatch_semaphore_signal(VuoSceneRenderer_vertexArraySemaphore);
}

/**
 * Produces a new @ref VuoSceneObject by rendering @c sceneObject using @c shader's GLSL vertex shader.
 *
 * `VuoSubmesh`es are left unchanged if they have an elementAssemblyMethod that differs from the shader's inputElementType.
 *
 * @threadAnyGL
 * (Additionally, the caller is responsible for ensuring that the same @c VuoSceneObjectRenderer is not used simultaneously on multiple threads.)
 */
VuoSceneObject VuoSceneObjectRenderer_draw(VuoSceneObjectRenderer sor, VuoSceneObject sceneObject)
{
	if (!sor)
		return VuoSceneObject_makeEmpty();

	VuoSceneObject sceneObjectCopy = VuoSceneObject_copy(sceneObject);

	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;
	CGLContextObj cgl_ctx = (CGLContextObj)sceneObjectRenderer->glContext;

	VuoSceneObject_apply(&sceneObjectCopy, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]) {
							 VuoSceneObjectRenderer_drawSingle(cgl_ctx, sceneObjectRenderer, currentObject, modelviewMatrix);
						 });

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Ensure commands are submitted before we try to use the generated object on another context.
	// https://b33p.net/kosada/node/10467
	glFlushRenderAPPLE();

	return sceneObjectCopy;
}

/**
 * Destroys and deallocates the image renderer.
 *
 * @threadAny
 */
void VuoSceneObjectRenderer_destroy(VuoSceneObjectRenderer sor)
{
	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;

	VuoShader_cleanupContext(sceneObjectRenderer->glContext);

	CGLContextObj cgl_ctx = (CGLContextObj)sceneObjectRenderer->glContext;

	VuoRelease(sceneObjectRenderer->shader);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	VuoGlTexture_release(GL_RGBA, 1, 1, sceneObjectRenderer->shamTexture, GL_TEXTURE_2D);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);	// handled by glDeleteFramebuffers
	glDeleteFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);

	glDeleteVertexArrays(1, &sceneObjectRenderer->vertexArray);

	glDisable(GL_RASTERIZER_DISCARD_EXT);

	glDeleteQueries(1, &sceneObjectRenderer->query);

	free(sceneObjectRenderer);
}
