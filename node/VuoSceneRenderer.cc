/**
 * @file
 * VuoSceneRenderer implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <list>

#include "VuoSceneRenderer.h"
#include "VuoSceneObject.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
//#import <OpenGL/gl3.h>
/// @todo After we drop 10.6 support, switch back to gl3 and remove the below 4 lines.  See also r15430 for shader changes.
#include <OpenGL/glext.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

#include <dispatch/dispatch.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSceneRenderer",
					 "dependencies" : [
						 "VuoGLContext"
					 ]
				 });
#endif
}

/**
 * GL Objects corresponding with a VuoVertices instance.
 */
struct VuoSceneRendererInternal_vertices
{
	GLuint vertexArray;
	GLuint positionBuffer;
	GLuint normalBuffer;
	GLuint tangentBuffer;
	GLuint bitangentBuffer;
	GLuint textureCoordinateBuffer;
	GLuint elementBuffer;
};

/**
 * GL Objects corresponding with a VuoSceneObject instance.
 */
class VuoSceneRendererInternal_object
{
public:
	std::list<VuoSceneRendererInternal_vertices> vertices;
	std::list<VuoSceneRendererInternal_object> childObjects;
};

/**
 * Internal state data for a VuoSceneRenderer instance.
 */
class VuoSceneRendererInternal
{
public:
	dispatch_semaphore_t scenegraphSemaphore; ///< Serializes access to other data in this structure.
	bool scenegraphValid;

	VuoSceneObject rootSceneObject;
	VuoSceneRendererInternal_object rootSceneObjectInternal;

	float projectionMatrix[16]; ///< Column-major 4x4 matrix
};

void VuoSceneRenderer_destroy(VuoSceneRenderer sceneRenderer);

/**
 * Creates a reference-counted object for rendering a scenegraph.
 *
 * May be called from any thread (doesn't require a GL Context).
 */
VuoSceneRenderer VuoSceneRenderer_make(void)
{
	VuoSceneRendererInternal *sceneRenderer = new VuoSceneRendererInternal;
	VuoRegister(sceneRenderer, VuoSceneRenderer_destroy);

	sceneRenderer->scenegraphSemaphore = dispatch_semaphore_create(1);
	sceneRenderer->scenegraphValid = false;

	return (VuoSceneRenderer)sceneRenderer;
}

/**
 * Sets up OpenGL state on the current GL Context.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_prepareContext(VuoSceneRenderer sceneRenderer)
{
	glEnable(GL_MULTISAMPLE);

	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * Recalculates the projection matrix based on the specified viewport @c width and @c height.
 *
 * May be called from any thread (doesn't require a GL Context).
 */
void VuoSceneRenderer_regenerateProjectionMatrix(VuoSceneRenderer sr, unsigned int width, unsigned int height)
{
	struct VuoSceneRendererInternal *sceneRenderer = (struct VuoSceneRendererInternal *)sr;

	float halfFieldOfView = (90. * M_PI/180.) / 2.;
	float aspectRatio = (float)width/(float)height;
	/// @todo Where to place clip planes?
	float nearClipDistance = .1;
	float farClipDistance = 10.;
	float cameraZ = 1.;

	sceneRenderer->projectionMatrix[ 0] = 1./tanf(halfFieldOfView);
	sceneRenderer->projectionMatrix[ 1] = 0;
	sceneRenderer->projectionMatrix[ 2] = 0;
	sceneRenderer->projectionMatrix[ 3] = 0;

	sceneRenderer->projectionMatrix[ 4] = 0;
	sceneRenderer->projectionMatrix[ 5] = aspectRatio/tanf(halfFieldOfView);
	sceneRenderer->projectionMatrix[ 6] = 0;
	sceneRenderer->projectionMatrix[ 7] = 0;

	sceneRenderer->projectionMatrix[ 8] = 0;
	sceneRenderer->projectionMatrix[ 9] = 0;
	sceneRenderer->projectionMatrix[10] = (farClipDistance+nearClipDistance)/(nearClipDistance-farClipDistance);
	sceneRenderer->projectionMatrix[11] = -1.;

	sceneRenderer->projectionMatrix[12] = 0;
	sceneRenderer->projectionMatrix[13] = 0;
	sceneRenderer->projectionMatrix[14] = 2.*farClipDistance*nearClipDistance/(nearClipDistance-farClipDistance);
	sceneRenderer->projectionMatrix[15] = 0;

	// Move the camera to (0,0,1) by shifting the scene in the opposite direction.
	float translationMatrix[4][4];
	VuoTransform_getMatrix(VuoTransform_makeEuler(VuoPoint3d_make(0,0,-1), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)), (float *)translationMatrix);
	float outputMatrix[4][4];
	VuoTransform_multiplyMatrices4x4((float *)translationMatrix, sceneRenderer->projectionMatrix, (float *)&outputMatrix);
	VuoTransform_copyMatrix4x4((float *)outputMatrix, sceneRenderer->projectionMatrix);
}

/**
 * Draws @c so (using the uploaded object names in @c soi).
 * Does not traverse child objects.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_drawSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16])
{
	if (!so.shader)
		return;

	glUseProgram(so.shader->glProgramName);
	{
		GLint projectionMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "projectionMatrix");
		glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix);

		GLint modelviewMatrixUniform = glGetUniformLocation(so.shader->glProgramName, "modelviewMatrix");
		glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);

		VuoShader_activateTextures(so.shader);

		unsigned int i = 1;
		for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi, ++i)
		{
			GLint positionAttribute = glGetAttribLocation(so.shader->glProgramName, "position");
			glBindBuffer(GL_ARRAY_BUFFER, (*vi).positionBuffer);
			glVertexAttribPointer(positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
			glEnableVertexAttribArray(positionAttribute);

			GLint normalAttribute = glGetAttribLocation(so.shader->glProgramName, "normal");
			if (normalAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).normalBuffer);
				glVertexAttribPointer(normalAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray(normalAttribute);
			}

			GLint tangentAttribute = glGetAttribLocation(so.shader->glProgramName, "tangent");
			if (tangentAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).tangentBuffer);
				glVertexAttribPointer(tangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray(tangentAttribute);
			}

			GLint bitangentAttribute = glGetAttribLocation(so.shader->glProgramName, "bitangent");
			if (bitangentAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).bitangentBuffer);
				glVertexAttribPointer(bitangentAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray(bitangentAttribute);
			}

			GLint textureCoordinateAttribute = glGetAttribLocation(so.shader->glProgramName, "textureCoordinate");
			if (textureCoordinateAttribute >= 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, (*vi).textureCoordinateBuffer);
				glVertexAttribPointer(textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d), (void*)0);
				glEnableVertexAttribArray(textureCoordinateAttribute);
			}

			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*vi).elementBuffer);
				VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);
				GLenum mode;
				if (vertices.elementAssemblyMethod == VuoVertices_IndividualTriangles)
					mode = GL_TRIANGLES;
				else if (vertices.elementAssemblyMethod == VuoVertices_TriangleStrip)
					mode = GL_TRIANGLE_STRIP;
				else if (vertices.elementAssemblyMethod == VuoVertices_TriangleFan)
					mode = GL_TRIANGLE_FAN;
				glDrawElements(mode, vertices.elementCount, GL_UNSIGNED_INT, (void*)0);
			}

			if (textureCoordinateAttribute >= 0)
				glDisableVertexAttribArray(textureCoordinateAttribute);
			if (bitangentAttribute >= 0)
				glDisableVertexAttribArray(bitangentAttribute);
			if (tangentAttribute >= 0)
				glDisableVertexAttribArray(tangentAttribute);
			if (normalAttribute >= 0)
				glDisableVertexAttribArray(normalAttribute);
			glDisableVertexAttribArray(positionAttribute);
		}

		VuoShader_deactivateTextures(so.shader);
	}
	glUseProgram(0);
}

/**
 * Draws @c so and its child objects.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_drawSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi, float projectionMatrix[16], float modelviewMatrix[16])
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
	VuoSceneRenderer_drawSceneObject(so, soi, projectionMatrix, compositeModelviewMatrix);

	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_drawSceneObjectsRecursively(childObject, &(*oi), projectionMatrix, compositeModelviewMatrix);
	}
}

/**
 * Renders the scene.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_draw(VuoSceneRenderer sr)
{
	struct VuoSceneRendererInternal *sceneRenderer = (struct VuoSceneRendererInternal *)sr;

	if (!sceneRenderer->scenegraphValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);
	VuoSceneRenderer_drawSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal, sceneRenderer->projectionMatrix, localModelviewMatrix);

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Draws all vertex normals in sceneRenderer-rootSceneObject.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_drawElement(VuoSceneRenderer sr, int element, double length)	// TODO Enum type for element to debug
{
	/// @todo update VuoSceneRenderer_draw() to call this for each VuoSceneObject when debugging
	struct VuoSceneRendererInternal *sceneRenderer = (struct VuoSceneRendererInternal *)sr;

	if (!sceneRenderer->scenegraphValid)
		return;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);

	unsigned int verticesListCount = VuoListGetCount_VuoVertices(sceneRenderer->rootSceneObject.verticesList);

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(sceneRenderer->projectionMatrix);


	glMatrixMode(GL_MODELVIEW);

	float modelViewMatrix[16];
	VuoTransform_getMatrix(sceneRenderer->rootSceneObject.transform, modelViewMatrix);
	glLoadMatrixf(modelViewMatrix);

	glBegin(GL_LINES);
	for (unsigned int i = 1; i <= verticesListCount; i++)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(sceneRenderer->rootSceneObject.verticesList, i);

		for(unsigned int m = 0; m < vertices.vertexCount; m++)
		{
			VuoPoint4d v = vertices.positions[m];
			VuoPoint4d n = (VuoPoint4d){0,0,1,1};

			switch(element)
			{
				case 0:	// normals
					n = vertices.normals[m];
					break;
				case 1:	// tangents
					n = vertices.tangents[m];
					break;
				case 2:	// bitangents
					n = vertices.bitangents[m];
					break;
			}

			n = VuoPoint4d_subtract(v, VuoPoint4d_multiply(n, length));

			glVertex3d(v.x, v.y, v.z);
			glVertex3d(n.x, n.y, n.z);
		}
	}
	glEnd();

	glLoadIdentity();

	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Uploads @c so to the GPU, and stores the uploaded object names in @c soi.
 * Does not traverse child objects.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_uploadSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi)
{
	if (!so.verticesList)
		return;

	unsigned int verticesListCount = VuoListGetCount_VuoVertices(so.verticesList);
	for (unsigned int i = 1; i <= verticesListCount; ++i)
	{
		VuoVertices vertices = VuoListGetValueAtIndex_VuoVertices(so.verticesList, i);
		VuoSceneRendererInternal_vertices v;

		glGenVertexArrays(1, &v.vertexArray);
		glBindVertexArray(v.vertexArray);

		glGenBuffers(1, &v.positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.positions, GL_STATIC_DRAW);

		glGenBuffers(1, &v.normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.normals, GL_STATIC_DRAW);

		glGenBuffers(1, &v.tangentBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.tangentBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.tangents, GL_STATIC_DRAW);

		glGenBuffers(1, &v.bitangentBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.bitangentBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.bitangents, GL_STATIC_DRAW);

		glGenBuffers(1, &v.textureCoordinateBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, v.textureCoordinateBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VuoPoint4d)*vertices.vertexCount, vertices.textureCoordinates, GL_STATIC_DRAW);

		glGenBuffers(1, &v.elementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v.elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*vertices.elementCount, vertices.elements, GL_STATIC_DRAW);

		soi->vertices.push_back(v);
	}
}

/**
 * Uploads @c so and its child objects to the GPU, and stores the uploaded object names in @c soi.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_uploadSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi)
{
	VuoSceneRenderer_uploadSceneObject(so, soi);

	if (!so.childObjects)
		return;

	unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	for (unsigned int i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRendererInternal_object childObjectInternal;

		VuoSceneRenderer_uploadSceneObjectsRecursively(childObject, &childObjectInternal);

		soi->childObjects.push_back(childObjectInternal);
	}
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObject.
 * Does not traverse child objects.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_releaseSceneObject(VuoSceneObject so, VuoSceneRendererInternal_object *soi)
{
	for (std::list<VuoSceneRendererInternal_vertices>::iterator vi = soi->vertices.begin(); vi != soi->vertices.end(); ++vi)
	{
		glDeleteVertexArrays(1, &(*vi).vertexArray);
		glDeleteBuffers(1, &(*vi).positionBuffer);
		glDeleteBuffers(1, &(*vi).normalBuffer);
		glDeleteBuffers(1, &(*vi).tangentBuffer);
		glDeleteBuffers(1, &(*vi).bitangentBuffer);
		glDeleteBuffers(1, &(*vi).textureCoordinateBuffer);
		glDeleteBuffers(1, &(*vi).elementBuffer);
	}
	soi->vertices.clear();

	CGLContextObj cglContext = CGLGetCurrentContext();
	VuoSceneObject_release(so);
	CGLSetCurrentContext(cglContext);
}

/**
 * Releases the GPU objects created by @c VuoSceneRenderer_uploadSceneObjectsRecursively.
 *
 * Must be called from a thread with an active GL Context.
 */
void VuoSceneRenderer_releaseSceneObjectsRecursively(VuoSceneObject so, VuoSceneRendererInternal_object *soi)
{
	unsigned int i = 1;
	for (std::list<VuoSceneRendererInternal_object>::iterator oi = soi->childObjects.begin(); oi != soi->childObjects.end(); ++oi, ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_releaseSceneObjectsRecursively(childObject, &(*oi));
	}

	soi->childObjects.clear();

	VuoSceneRenderer_releaseSceneObject(so, soi);
}

/**
 * Deeply retains @c so.
 *
 * May be called from any thread (doesn't require a GL Context).
 */
void VuoSceneRenderer_retainSceneObjectsRecursively(VuoSceneObject so)
{
	VuoSceneObject_retain(so);

	if (!so.childObjects)
		return;

	unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
	for (unsigned int i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		VuoSceneRenderer_retainSceneObjectsRecursively(childObject);
	}
}

/**
 * Changes the scenegraph to be rendered.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
void VuoSceneRenderer_setRootSceneObject(VuoSceneRenderer sr, VuoSceneObject rootSceneObject)
{
	struct VuoSceneRendererInternal *sceneRenderer = (struct VuoSceneRendererInternal *)sr;

	dispatch_semaphore_wait(sceneRenderer->scenegraphSemaphore, DISPATCH_TIME_FOREVER);
	VuoSceneRenderer_retainSceneObjectsRecursively(rootSceneObject);

	VuoGlContext_use();

	// Release the old scenegraph.
	if (sceneRenderer->scenegraphValid)
		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal);

	// Upload the new scenegraph.
	sceneRenderer->rootSceneObject = rootSceneObject;
	VuoSceneRenderer_uploadSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal);

	VuoGlContext_disuse();

	sceneRenderer->scenegraphValid = true;
	dispatch_semaphore_signal(sceneRenderer->scenegraphSemaphore);
}

/**
 * Destroys and deallocates the scene renderer.
 *
 * May be called from any thread (automatically uses and disuses a GL Context).
 */
void VuoSceneRenderer_destroy(VuoSceneRenderer sr)
{
	struct VuoSceneRendererInternal *sceneRenderer = (struct VuoSceneRendererInternal *)sr;

	VuoGlContext_use();

	if (sceneRenderer->scenegraphValid)
		VuoSceneRenderer_releaseSceneObjectsRecursively(sceneRenderer->rootSceneObject, &sceneRenderer->rootSceneObjectInternal);

	VuoGlContext_disuse();

	dispatch_release(sceneRenderer->scenegraphSemaphore);
	VuoSceneObject_release(sceneRenderer->rootSceneObject);

	free(sceneRenderer);
}
