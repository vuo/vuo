/**
 * @file
 * VuoSceneObjectRenderer implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"

#include <Block.h>
#include <CoreServices/CoreServices.h>
#include <OpenGL/CGLMacro.h>
/// @{ Stub.
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoSceneObjectRenderer",
					 "dependencies" : [
						 "VuoSceneObject",
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
	GLint textureCoordinate;
	GLint color;

	unsigned int expectedOutputPrimitiveCount;
	bool mayChangeOutputPrimitiveCount;
} VuoSceneObjectRenderer_Attributes;

/**
 * Internal state data for a VuoSceneObjectRenderer instance.
 */
struct VuoSceneObjectRendererInternal
{
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
VuoSceneObjectRenderer VuoSceneObjectRenderer_make(VuoShader shader)
{
	if (!VuoShader_isTransformFeedback(shader))
	{
		VUserLog("Error '%s' is not a transform feedback shader.", shader->name);
		return NULL;
	}

	__block struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)malloc(sizeof(struct VuoSceneObjectRendererInternal));
	VuoRegister(sceneObjectRenderer, VuoSceneObjectRenderer_destroy);

	if (VuoSceneObjectRenderer_usingGPU())
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			// Fetch the shader's attribute locations
			bool havePoints    = VuoShader_getAttributeLocations(shader, VuoMesh_Points,              cgl_ctx, &sceneObjectRenderer->pointAttributes.position,    &sceneObjectRenderer->pointAttributes.normal,    &sceneObjectRenderer->pointAttributes.textureCoordinate,    &sceneObjectRenderer->pointAttributes.color   );
			bool haveLines     = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualLines,     cgl_ctx, &sceneObjectRenderer->lineAttributes.position,     &sceneObjectRenderer->lineAttributes.normal,     &sceneObjectRenderer->lineAttributes.textureCoordinate,     &sceneObjectRenderer->lineAttributes.color    );
			bool haveTriangles = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &sceneObjectRenderer->triangleAttributes.position, &sceneObjectRenderer->triangleAttributes.normal, &sceneObjectRenderer->triangleAttributes.textureCoordinate, &sceneObjectRenderer->triangleAttributes.color);
			if (!havePoints || !haveLines || !haveTriangles)
			{
				VUserLog("Error: '%s' is missing programs for: %s %s %s", shader->name, havePoints? "" : "points", haveLines? "" : "lines", haveTriangles? "" : "triangles");
				free(sceneObjectRenderer);
				sceneObjectRenderer = NULL;
				return;
			}

			sceneObjectRenderer->pointAttributes.expectedOutputPrimitiveCount     = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_Points);
			sceneObjectRenderer->pointAttributes.mayChangeOutputPrimitiveCount    = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_Points);

			sceneObjectRenderer->lineAttributes.expectedOutputPrimitiveCount      = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_IndividualLines);
			sceneObjectRenderer->lineAttributes.mayChangeOutputPrimitiveCount     = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_IndividualLines);

			sceneObjectRenderer->triangleAttributes.expectedOutputPrimitiveCount  = VuoShader_getExpectedOutputPrimitiveCount (shader, VuoMesh_IndividualTriangles);
			sceneObjectRenderer->triangleAttributes.mayChangeOutputPrimitiveCount = VuoShader_getMayChangeOutputPrimitiveCount(shader, VuoMesh_IndividualTriangles);

			sceneObjectRenderer->shader = shader;
			VuoRetain(sceneObjectRenderer->shader);

			glGenVertexArrays(1, &sceneObjectRenderer->vertexArray);

			// https://stackoverflow.com/questions/24112671/transform-feedback-without-a-framebuffer
			sceneObjectRenderer->shamTexture = VuoGlTexturePool_use(cgl_ctx, VuoGlTexturePool_Allocate, GL_TEXTURE_2D, GL_RGBA, 1, 1, GL_BGRA, NULL);
			VuoGlTexture_retain(sceneObjectRenderer->shamTexture, NULL, NULL);
			glGenFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, sceneObjectRenderer->shamFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneObjectRenderer->shamTexture, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glGenQueries(1, &sceneObjectRenderer->query);
		});

	return (VuoSceneObjectRenderer)sceneObjectRenderer;
}

/**
 * Helper for @ref VuoSceneObjectRenderer_draw.
 * Applies a shader to a single @c sceneObject's VuoMesh (ignoring its childObjects).
 */
static void VuoSceneObjectRenderer_drawSingle(CGLContextObj cgl_ctx, struct VuoSceneObjectRendererInternal *sceneObjectRenderer, VuoSceneObject sceneObject, float modelviewMatrix[16])
{
	VuoMesh mesh = VuoSceneObject_getMesh(sceneObject);
	if (!mesh)
		return;

	VuoMesh_ElementAssemblyMethod outputPrimitiveMode = VuoMesh_getExpandedPrimitiveMode(VuoMesh_getElementAssemblyMethod(mesh));
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

	unsigned int vertexCount, combinedBuffer, elementCount, elementBuffer;
	void *normalOffset, *textureCoordinateOffset, *colorOffset;
	VuoMesh_getGPUBuffers(mesh, &vertexCount, &combinedBuffer, &normalOffset, &textureCoordinateOffset, &colorOffset, &elementCount, &elementBuffer);


		// Attach the input combinedBuffer for rendering.
		glBindBuffer(GL_ARRAY_BUFFER, combinedBuffer);

		VuoGlProgram program;
		if (!VuoShader_activate(sceneObjectRenderer->shader, VuoMesh_getElementAssemblyMethod(mesh), cgl_ctx, &program))
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


		int stride = sizeof(float) * 3;
		glEnableVertexAttribArray(attributes.position);
		glVertexAttribPointer(attributes.position, 3 /* XYZ */, GL_FLOAT, GL_FALSE, stride, (void*)0);
		bool hasNormals = normalOffset && attributes.normal >= 0;
		if (hasNormals)
		{
			glEnableVertexAttribArray(attributes.normal);
			glVertexAttribPointer(attributes.normal, 3 /* XYZ */, GL_FLOAT, GL_FALSE, stride, normalOffset);
		}
		bool hasTextureCoordinates = textureCoordinateOffset && attributes.textureCoordinate >= 0;
		if (hasTextureCoordinates)
		{
			glEnableVertexAttribArray(attributes.textureCoordinate);
			glVertexAttribPointer(attributes.textureCoordinate, 2 /* XY */, GL_FLOAT, GL_FALSE, sizeof(float) * 2, textureCoordinateOffset);
		}
		bool hasColors = colorOffset && attributes.color >= 0;
		if (hasColors)
		{
			glEnableVertexAttribArray(attributes.color);
			glVertexAttribPointer(attributes.color, 4 /* RGBA */, GL_FLOAT, GL_FALSE, sizeof(float) * 4, colorOffset);
		}


		// Attach the input elementBuffer for rendering.
		if (elementBuffer)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);


		// Create and attach the output combinedBuffer.
		// The output buffer will always contain all 4 vertex attributes (position, normal, textureCoordinate, color).
		unsigned long outputVertexCount = VuoMesh_getSplitVertexCount(mesh) * attributes.expectedOutputPrimitiveCount;
		unsigned long combinedOutputBufferSize = sizeof(float) * (3 + 3 + 2 + 4) * outputVertexCount;
		GLuint combinedOutputBuffer = VuoGlPool_use(cgl_ctx, VuoGlPool_ArrayBuffer, combinedOutputBufferSize);
		VuoGlPool_retain(combinedOutputBuffer);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBuffer);
//		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBufferSize, NULL, GL_STATIC_READ);
//		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, combinedOutputBuffer);

		int offset = 0;
		int size = sizeof(float) * 3 * outputVertexCount;
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, combinedOutputBuffer, 0, size);
		offset += size;
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 1, combinedOutputBuffer, offset, size);
		offset += size;
		size = sizeof(float) * 2 * outputVertexCount;
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 2, combinedOutputBuffer, offset, size);
		offset += size;
		size = sizeof(float) * 4 * outputVertexCount;
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 3, combinedOutputBuffer, offset, size);



		// Execute the filter.
		GLenum mode = VuoMesh_getGlMode(mesh);
		if (attributes.mayChangeOutputPrimitiveCount)
			glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT, sceneObjectRenderer->query);
		glBeginTransformFeedbackEXT(outputPrimitiveGlMode);

#ifdef VUO_PROFILE
	GLuint timeElapsedQuery;
	glGenQueries(1, &timeElapsedQuery);
	glBeginQuery(GL_TIME_ELAPSED_EXT, timeElapsedQuery);
#endif

		unsigned long completeInputElementCount = VuoMesh_getCompleteElementCount(mesh);
		if (elementCount)
			glDrawElements(mode, completeInputElementCount, GL_UNSIGNED_INT, (void*)0);
		else if (vertexCount)
			glDrawArrays(mode, 0, completeInputElementCount);

#ifdef VUO_PROFILE
	double seconds;
	glEndQuery(GL_TIME_ELAPSED_EXT);
	GLuint nanoseconds;
	glGetQueryObjectuiv(timeElapsedQuery, GL_QUERY_RESULT, &nanoseconds);
	seconds = ((double)nanoseconds) / NSEC_PER_SEC;
	glDeleteQueries(1, &timeElapsedQuery);

	double objectPercent = seconds / (1./60.) * 100.;
	VLog("%6.2f %% of 60 Hz frame	%s (%s)", objectPercent, sceneObject->name, sceneObjectRenderer->shader->name);
#endif


		glEndTransformFeedbackEXT();
		if (attributes.mayChangeOutputPrimitiveCount)
			glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0);

		if (elementBuffer)
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if (hasColors)
			glDisableVertexAttribArray(attributes.color);
		if (hasTextureCoordinates)
			glDisableVertexAttribArray(attributes.textureCoordinate);
		if (hasNormals)
			glDisableVertexAttribArray(attributes.normal);
		glDisableVertexAttribArray(attributes.position);

		VuoShader_deactivate(sceneObjectRenderer->shader, VuoMesh_getElementAssemblyMethod(mesh), cgl_ctx);


		GLuint actualVertexCount = 0;
		if (attributes.mayChangeOutputPrimitiveCount)
		{
			glGetQueryObjectuiv(sceneObjectRenderer->query, GL_QUERY_RESULT, &actualVertexCount);
			actualVertexCount *= primitiveVertexMultiplier;
		}
		else
			actualVertexCount = outputVertexCount;


#if 0 // NOCOMMIT
		// Print out the result of the filter, for debugging.
		VLog("inputElements=%lu  actualVertexCount=%d  normals=%d  textureCoordinates=%d  colors=%d", completeInputElementCount, actualVertexCount, hasNormals, hasTextureCoordinates, hasColors);
		glFlush();
		GLfloat feedback[actualVertexCount * (3 + 3 + 2 + 4)];
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBuffer);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, sizeof(feedback), feedback);
		for (int i = 0; i < actualVertexCount; ++i)
			fprintf(stderr, "\t%3d = pos %5.2f %5.2f %5.2f    normal %5.2f %5.2f %5.2f    tc %5.2f %5.2f    color %5.2f %5.2f %5.2f %5.2f\n", i,
				feedback[i * 3], feedback[i * 3 + 1], feedback[i * 3 + 2],
				feedback[(outputVertexCount + i) * 3], feedback[(outputVertexCount + i) * 3 + 1], feedback[(outputVertexCount + i) * 3 + 2],
				feedback[outputVertexCount * (3 + 3) + i * 2], feedback[outputVertexCount * (3 + 3) + i * 2 + 1],
				feedback[outputVertexCount * (3 + 3 + 2) + i * 4], feedback[outputVertexCount * (3 + 3 + 2) + i * 4 + 1], feedback[outputVertexCount * (3 + 3 + 2) + i * 4 + 2], feedback[outputVertexCount * (3 + 3 + 2) + i * 4 + 3]);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0);
#endif

		// https://vuo.org/node/1571
		// https://b33p.net/kosada/node/12431
		// The output buffer will always have room for all 4 vertex attributes,
		// though (depending on input) some might not contain contain useful data.

		// https://b33p.net/kosada/vuo/vuo/-/issues/18103
		// Though normals are technically optional
		// (since they're omitted for 2D-only meshes such as that used by VuoImageRenderer),
		// we expect that all 3D object filters will produce valid normals
		// (even if they don't use the input mesh's normals),
		// since composition authors may use a lighting shader or subsequent 3D object filters.

		VuoMesh newMesh = VuoMesh_makeFromGPUBuffers(
			actualVertexCount, combinedOutputBuffer, combinedOutputBufferSize,
									(void *)(sizeof(float) * (3        ) * outputVertexCount),
			hasTextureCoordinates ? (void *)(sizeof(float) * (3 + 3    ) * outputVertexCount) : nullptr,
			hasColors             ? (void *)(sizeof(float) * (3 + 3 + 2) * outputVertexCount) : nullptr,
			0, 0, 0, // Since transform feedback doesn't provide an elementBuffer, render this submesh using glDrawArrays().
			outputPrimitiveMode);
		VuoMesh_setFaceCulling(newMesh, VuoMesh_getFaceCulling(mesh));
		VuoMesh_setPrimitiveSize(newMesh, VuoMesh_getPrimitiveSize(mesh));

	VuoSceneObject_setMesh(sceneObject, newMesh);
}

/// Returns the vertex index to use.
#define ELEM(i) (elementCount ? elements[i] : i)

/**
 * Helper for @ref VuoSceneObjectRenderer_drawSingleOnCPU.
 */
static inline void VuoSceneObjectRenderer_copyElement(VuoMesh mesh, int start, float *source, unsigned int elementCount, unsigned int *elements, VuoMesh_ElementAssemblyMethod elementAssemblyMethod, int verticesPerPrimitive, int floatsPerVertex, float *destination, float *defaultValues)
{
	if (!source)
	{
		for (int i = 0; i < verticesPerPrimitive * floatsPerVertex; ++i)
			destination[i] = defaultValues[i % floatsPerVertex];
		return;
	}

	if (elementAssemblyMethod == VuoMesh_IndividualTriangles
	 || elementAssemblyMethod == VuoMesh_IndividualLines
	 || elementAssemblyMethod == VuoMesh_Points)
	{
		if (elementCount)
		{
			unsigned int *e = elements + start;
			for (int i = 0; i < verticesPerPrimitive; ++i)
				for (int j = 0; j < floatsPerVertex; ++j)
					destination[i * floatsPerVertex + j] = source[*(e + i) * floatsPerVertex + j];
		}
		else
		{
			for (int i = 0; i < verticesPerPrimitive; ++i)
				for (int j = 0; j < floatsPerVertex; ++j)
					destination[i * floatsPerVertex + j] = source[(start + i) * floatsPerVertex + j];
		}
	}
	else if (elementAssemblyMethod == VuoMesh_TriangleStrip)
	{
		// Expand the triangle strip to individual triangles.
		if ((start / 3) % 2 == 0)
		{
			for (int i = 0; i < verticesPerPrimitive; ++i)
				for (int j = 0; j < floatsPerVertex; ++j)
					destination[i * floatsPerVertex + j] = source[ELEM(start/3 + i) * floatsPerVertex + j];
		}
		else
		{
			for (int j = 0; j < floatsPerVertex; ++j)
			{
				destination[0 * floatsPerVertex + j] = source[ELEM(start/3 + 1) * floatsPerVertex + j];
				destination[1 * floatsPerVertex + j] = source[ELEM(start/3    ) * floatsPerVertex + j];
				destination[2 * floatsPerVertex + j] = source[ELEM(start/3 + 2) * floatsPerVertex + j];
			}
		}
	}
	else if (elementAssemblyMethod == VuoMesh_TriangleFan)
	{
		// Expand the triangle fan to individual triangles.
		for (int j = 0; j < floatsPerVertex; ++j)
		{
			destination[0 * floatsPerVertex + j] = source[ELEM(0          ) * floatsPerVertex + j];
			destination[1 * floatsPerVertex + j] = source[ELEM(start/3 + 1) * floatsPerVertex + j];
			destination[2 * floatsPerVertex + j] = source[ELEM(start/3 + 2) * floatsPerVertex + j];
		}
	}
	else if (elementAssemblyMethod == VuoMesh_LineStrip)
	{
		// Expand the line strip to individual lines.
		for (int j = 0; j < floatsPerVertex; ++j)
		{
			destination[0 * floatsPerVertex + j] = source[ELEM(start/2    ) * floatsPerVertex + j];
			destination[1 * floatsPerVertex + j] = source[ELEM(start/2 + 1) * floatsPerVertex + j];
		}
	}
}

/**
 * Helper for @ref VuoSceneObjectRenderer_draw.
 * Applies `cpuGeometryOperator` to a single `sceneObject`'s VuoMesh (ignoring its childObjects).
 */
static void VuoSceneObjectRenderer_drawSingleOnCPU(VuoSceneObject sceneObject, float modelviewMatrix[16], VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator)
{
	VuoMesh mesh = VuoSceneObject_getMesh(sceneObject);
	if (!mesh)
		return;

	float *inputPositions, *inputNormals, *inputTextureCoordinates, *inputColors;
	unsigned int elementCount, *elements;
	VuoMesh_getCPUBuffers(mesh, nullptr, &inputPositions, &inputNormals, &inputTextureCoordinates, &inputColors, &elementCount, &elements);

	float modelMatrixInverse[16];
	VuoTransform_invertMatrix4x4(modelviewMatrix, modelMatrixInverse);



//	unsigned int vertexCount;
//	VuoMesh_getCPUBuffers(mesh, &vertexCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
//	for (int i = 0; i < vertexCount; ++i)
//		VLog(" input[%2d] = %f",i,inputPositions[i]);




		unsigned int inputCount = VuoMesh_getSplitVertexCount(mesh);

		unsigned int allocatedVertices = inputCount;
		float *newPositions, *newNormals, *newTextureCoordinates, *newColors;
		VuoMesh_allocateCPUBuffers(allocatedVertices, &newPositions, &newNormals, &newTextureCoordinates, &newColors, 0, nullptr);
		unsigned int newVertexCount = 0;

		int verticesPerPrimitive;
		VuoMesh_ElementAssemblyMethod elementAssemblyMethod = VuoMesh_getElementAssemblyMethod(mesh);
		if (elementAssemblyMethod == VuoMesh_IndividualTriangles
		 || elementAssemblyMethod == VuoMesh_TriangleStrip
		 || elementAssemblyMethod == VuoMesh_TriangleFan)
			verticesPerPrimitive = 3;
		else if (elementAssemblyMethod == VuoMesh_IndividualLines
			  || elementAssemblyMethod == VuoMesh_LineStrip)
			verticesPerPrimitive = 2;
		else if (elementAssemblyMethod == VuoMesh_Points)
			verticesPerPrimitive = 1;

		float defaultPosition[3] = {0,0,0};
		float defaultNormal[3] = {0,0,1};
		float defaultColor[4] = {1,1,1,1};

		for (unsigned int e = 0; e < inputCount; e += verticesPerPrimitive)
		{
			if (newVertexCount + VuoSceneObjectRenderer_maxOutputVertices > allocatedVertices)
			{
				allocatedVertices *= 2;
				newPositions          = (float *)realloc(newPositions,          sizeof(float) * 3 * allocatedVertices);
				newNormals            = (float *)realloc(newNormals,            sizeof(float) * 3 * allocatedVertices);
				newTextureCoordinates = (float *)realloc(newTextureCoordinates, sizeof(float) * 2 * allocatedVertices);
				newColors             = (float *)realloc(newColors,             sizeof(float) * 4 * allocatedVertices);
			}

			float *positions          = newPositions          + newVertexCount * 3;
			float *normals            = newNormals            + newVertexCount * 3;
			float *textureCoordinates = newTextureCoordinates + newVertexCount * 2;
			float *colors             = newColors             + newVertexCount * 4;
			VuoSceneObjectRenderer_copyElement(mesh, e, inputPositions,          elementCount, elements, elementAssemblyMethod, verticesPerPrimitive, 3, positions,          defaultPosition);
			VuoSceneObjectRenderer_copyElement(mesh, e, inputNormals,            elementCount, elements, elementAssemblyMethod, verticesPerPrimitive, 3, normals,            defaultNormal);
			VuoSceneObjectRenderer_copyElement(mesh, e, inputTextureCoordinates, elementCount, elements, elementAssemblyMethod, verticesPerPrimitive, 2, textureCoordinates, defaultPosition);
			VuoSceneObjectRenderer_copyElement(mesh, e, inputColors,             elementCount, elements, elementAssemblyMethod, verticesPerPrimitive, 4, colors,             defaultColor);

			int vertexCount = verticesPerPrimitive;

//			for (int i = 0; i < vertexCount; ++ i)
//				VLog("prim %3d vertex %d    pos %f,%f,%f    norm=%f,%f,%f    tc=%f,%f    color=%f,%f,%f,%f",e,i,
//					 positions[i*3],positions[i*3+1],positions[i*3+2],
//					 normals[i*3],normals[i*3+1],normals[i*3+2],
//					 textureCoordinates[i*2],textureCoordinates[i*2+1],
//					 colors[i*4],colors[i*4+1],colors[i*4+2],colors[i*4+3]);

			cpuGeometryOperator(modelviewMatrix, modelMatrixInverse, &vertexCount, positions, normals, textureCoordinates, colors);

//			for (int i = 0; i < vertexCount; ++ i)
//				VLog("prim %3d vertex %d    pos %f,%f,%f    norm=%f,%f,%f    tc=%f,%f    color=%f,%f,%f,%f",e,i,
//					 positions[i*3],positions[i*3+1],positions[i*3+2],
//					 normals[i*3],normals[i*3+1],normals[i*3+2],
//					 textureCoordinates[i*2],textureCoordinates[i*2+1],
//					 colors[i*4],colors[i*4+1],colors[i*4+2],colors[i*4+3]);

			if (vertexCount < 0 || vertexCount > VuoSceneObjectRenderer_maxOutputVertices)
			{
				VUserLog("Error: cpuGeometryOperator must output between 0 and %d vertices.", VuoSceneObjectRenderer_maxOutputVertices);
				return;
			}

			if (vertexCount % verticesPerPrimitive)
			{
				VUserLog("Error: When %d vertices are input to cpuGeometryOperator, it must output a multiple of %d vertices.", verticesPerPrimitive, verticesPerPrimitive);
				return;
			}

			newVertexCount += vertexCount;
		}


//		for (int i = 0; i < newVertexCount; ++i)
//			VLog("output[%2d] = %f",i,newPositions[i]);


		bool originalMeshHasTextureCoordinates = inputTextureCoordinates;
		VuoMesh newMesh = VuoMesh_makeFromCPUBuffers(newVertexCount, newPositions, newNormals, originalMeshHasTextureCoordinates ? newTextureCoordinates : nullptr, newColors,
			0, nullptr,
			elementAssemblyMethod);
		if (!originalMeshHasTextureCoordinates)
			free(newTextureCoordinates);
		VuoMesh_setFaceCulling(newMesh, VuoMesh_getFaceCulling(mesh));
		VuoMesh_setPrimitiveSize(newMesh, VuoMesh_getPrimitiveSize(mesh));

	VuoSceneObject_setMesh(sceneObject, newMesh);
}

/**
 * Converts @ref VuoSceneObjectRenderer_Deformer into @ref VuoSceneObjectRenderer_CPUGeometryOperator,
 * to make it easier to implemnet common 3D mesh filters.
 *
 * The caller is responsible for calling `Block_release` on the returned block.
 *
 * @version200New
 */
VuoSceneObjectRenderer_CPUGeometryOperator VuoSceneObjectRenderer_makeDeformer(VuoSceneObjectRenderer_Deformer deform)
{
	return Block_copy(^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
		VuoPoint3d positionInScene[3];
		VuoPoint3d deformedPositionInScene[3];
		VuoPoint3d normalInScene[3];
		VuoPoint2d textureCoordinate[3];
		for (int i = 0; i < *vertexCount; ++i)
		{
			// Keep this in sync with deform.glsl.

			// Position ============================================================

			// Transform into worldspace.
			VuoPoint3d position = VuoPoint3d_makeFromArray(&positions[i * 3]);
			VuoPoint3d normal   = VuoPoint3d_makeFromArray(&normals[i * 3]);
			textureCoordinate[i] = VuoPoint2d_makeFromArray(&textureCoordinates[i * 2]);
			positionInScene[i] = VuoTransform_transformPoint(modelMatrix, position);
			normalInScene[i]   = VuoPoint3d_normalize(VuoTransform_transformVector(modelMatrix, normal));

//			VLog("vertex %d    pos %f,%f,%f    norm=%f,%f,%f",i,
//				 positions[i*3],positions[i*3+1],positions[i*3+2],
//				 normals[i*3],normals[i*3+1],normals[i*3+2]);

			// Apply the deformation.
			VuoPoint3d deformedPosition = deform(positionInScene[i],
												 normalInScene[i],
												 textureCoordinate[i]);
			deformedPositionInScene[i] = deformedPosition;

			// Transform back into modelspace.
			VuoPoint3d positionInModelspace = VuoTransform_transformPoint(modelMatrixInverse, deformedPosition);
			VuoPoint3d_setArray(&positions[i * 3], positionInModelspace);
		}


		// Normal ==============================================================

		// First, find the tangent and bitangent vectors at the original (pre-deformation) position.
		VuoPoint3d tangentInScene[3], bitangentInScene[3];
		if (*vertexCount == 3)
		{
			// Based on "Computing Tangent Space Basis Vectors for an Arbitrary Mesh" by Eric Lengyel,
			// Terathon Software 3D Graphics Library, 2001.
			// https://web.archive.org/web/20160306000702/http://www.terathon.com/code/tangent.html

			VuoPoint3d tan1[3], tan2[3];
			bzero(tan1, sizeof(VuoPoint3d) * 3);
			bzero(tan2, sizeof(VuoPoint3d) * 3);

			VuoPoint3d v1 = positionInScene[0];
			VuoPoint3d v2 = positionInScene[1];
			VuoPoint3d v3 = positionInScene[2];

			VuoPoint2d w1 = textureCoordinate[0];
			VuoPoint2d w2 = textureCoordinate[1];
			VuoPoint2d w3 = textureCoordinate[2];

			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;

			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;

			float r = 1.0F / (s1 * t2 - s2 * t1);
			VuoPoint3d sdir = (VuoPoint3d){
				(t2 * x1 - t1 * x2) * r,
				(t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r};
			VuoPoint3d tdir = (VuoPoint3d){
				(s1 * x2 - s2 * x1) * r,
				(s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r};

			tan1[0] += sdir;
			tan1[1] += sdir;
			tan1[2] += sdir;

			tan2[0] += tdir;
			tan2[1] += tdir;
			tan2[2] += tdir;

			for (int i = 0; i < *vertexCount; ++i)
			{
				VuoPoint3d n = normalInScene[i];
				VuoPoint3d t = tan1[i];
				VuoPoint3d t2 = tan2[i];

				// Gram-Schmidt orthogonalize
				tangentInScene[i]   = VuoPoint3d_normalize(t  - n * VuoPoint3d_dotProduct(n, t));
				bitangentInScene[i] = VuoPoint3d_normalize(t2 - n * VuoPoint3d_dotProduct(n, t2));
			}
		}
		else
			for (int i = 0; i < *vertexCount; ++i)
			{
				tangentInScene[i]   = (VuoPoint3d){1,0,0};
				bitangentInScene[i] = (VuoPoint3d){0,1,0};
			}

		// Next, apply the deformation to neighboring positions.
		// Based on https://web.archive.org/web/20170202071451/https://http.developer.nvidia.com/GPUGems/gpugems_ch42.html
		// section "An Approximate Numerical Technique".
		for (int i = 0; i < *vertexCount; ++i)
		{
			const float scale = .01;
			VuoPoint3d deformedAlongTangent   = deform(positionInScene[i] + tangentInScene[i] * scale,
													   normalInScene[i],
													   textureCoordinate[i] + (VuoPoint2d){scale, 0});
			VuoPoint3d deformedAlongBitangent = deform(positionInScene[i] + bitangentInScene[i] * scale,
													   normalInScene[i],
													   textureCoordinate[i] + (VuoPoint2d){0, scale});

			VuoPoint3d deformedPosition = deformedPositionInScene[i];

			// Calculate the orthonormal basis of the tangent plane at deformedPosition.
			VuoPoint3d   deformedTangent = deformedAlongTangent   - deformedPosition;
			VuoPoint3d deformedBitangent = deformedAlongBitangent - deformedPosition;
			VuoPoint3d    deformedNormal = VuoPoint3d_crossProduct(deformedTangent, deformedBitangent);

			// Transform back into modelspace.
			VuoPoint3d deformedNormalInModelspace = VuoPoint3d_normalize(VuoTransform_transformVector(modelMatrixInverse, deformedNormal));

			VuoPoint3d_setArray(&normals[i * 3], deformedNormalInModelspace);
		}
	});
}

/**
 * Returns true if this library will be using the GPU for transform feedback.
 *
 * @version200New
 */
bool VuoSceneObjectRenderer_usingGPU(void)
{
	static bool gpuTransformFeedback;
	static dispatch_once_t once = 0;
	dispatch_once(&once, ^{
		// If the user set the `gpuTransformFeedback` preference, use it.
		Boolean overridden = false;
		gpuTransformFeedback = (int)CFPreferencesGetAppIntegerValue(CFSTR("gpuTransformFeedback"), CFSTR("org.vuo.Editor"), &overridden);

		if (!overridden)
		{
			// https://b33p.net/kosada/node/13622
			// Some GPUs / GPU drivers have broken support for transform feedback,
			// so use the CPU fallbacks instead.
			__block const char *renderer;
			VuoGlContext_perform(^(CGLContextObj cgl_ctx){
				renderer = (const char *)glGetString(GL_RENDERER);
			});

			if (strncmp(renderer, "Intel", 5) == 0
			 || strncmp(renderer, "AMD ", 4) == 0
			 || strncmp(renderer, "NVIDIA GeForce 9400M", 4) == 0
			 || strncmp(renderer, "Apple ", 6) == 0
			 || strncmp(renderer, "ATI ", 4) == 0)
				gpuTransformFeedback = false;
			else
				// NVIDIA (except 9400M) seems to be the only GPU whose transform feedback works consistently.
				gpuTransformFeedback = true;
		}

		VUserLog("gpuTransformFeedback = %d", gpuTransformFeedback);
	});

	return gpuTransformFeedback;
}

/**
 * Produces a new @ref VuoSceneObject by rendering `sceneObject` using either:
 *
 *    - `shader`'s GLSL vertex shader (if the current GPU adequately supports transform feedback)
 *    - or `cpuGeometryOperator` (if the current GPU has trouble with transform feedback)
 *
 * `VuoSubmesh`es are left unchanged if they have an elementAssemblyMethod that differs from the shader's inputElementType.
 *
 * `cpuGeometryOperator`'s parameters serve as both input and output.
 * It may decrease (down to 0) or increase (up to @ref VuoSceneObjectRenderer_maxOutputVertices) the number of vertices it outputs.
 *
 * @threadAnyGL
 * (Additionally, the caller is responsible for ensuring that the same @c VuoSceneObjectRenderer is not used simultaneously on multiple threads.)
 *
 * @version200Changed{Added `cpuGeometryOperator` argument.}
 */
VuoSceneObject VuoSceneObjectRenderer_draw(VuoSceneObjectRenderer sor, VuoSceneObject sceneObject, VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator)
{
	if (!sor)
		return nullptr;

	if (VuoSceneObjectRenderer_usingGPU())
	{
		__block VuoSceneObject sceneObjectCopy = VuoSceneObject_copy(sceneObject);

		struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			glEnable(GL_RASTERIZER_DISCARD_EXT);
			glBindVertexArray(sceneObjectRenderer->vertexArray);
			glBindFramebuffer(GL_FRAMEBUFFER, sceneObjectRenderer->shamFramebuffer);

			VuoSceneObject_apply(sceneObjectCopy, ^(VuoSceneObject currentObject, float modelviewMatrix[16]) {
				VuoSceneObjectRenderer_drawSingle(cgl_ctx, sceneObjectRenderer, currentObject, modelviewMatrix);
			});

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindVertexArray(0);
			glDisable(GL_RASTERIZER_DISCARD_EXT);

			// Ensure commands are submitted before we try to use the generated object on another context.
			// https://b33p.net/kosada/node/10467
			glFlushRenderAPPLE();
		});

		return sceneObjectCopy;
	}

	else
	{
		VuoSceneObject sceneObjectCopy = VuoSceneObject_copy(sceneObject);

		VuoSceneObject_apply(sceneObjectCopy, ^(VuoSceneObject currentObject, float modelviewMatrix[16]) {
			VuoSceneObjectRenderer_drawSingleOnCPU(currentObject, modelviewMatrix, cpuGeometryOperator);
		});

		return sceneObjectCopy;
	}
}

/**
 * Destroys and deallocates the image renderer.
 *
 * @threadAny
 */
void VuoSceneObjectRenderer_destroy(VuoSceneObjectRenderer sor)
{
	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;

	if (VuoSceneObjectRenderer_usingGPU())
		VuoGlContext_perform(^(CGLContextObj cgl_ctx){
			VuoRelease(sceneObjectRenderer->shader);

			glBindFramebuffer(GL_FRAMEBUFFER, sceneObjectRenderer->shamFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			VuoGlTexture_release(VuoGlTexturePool_Allocate, GL_TEXTURE_2D, GL_RGBA, 1, 1, sceneObjectRenderer->shamTexture);
//			glBindFramebuffer(GL_FRAMEBUFFER, 0);	// handled by glDeleteFramebuffers
			glDeleteFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);

			glDeleteVertexArrays(1, &sceneObjectRenderer->vertexArray);

			glDeleteQueries(1, &sceneObjectRenderer->query);
		});

	free(sceneObjectRenderer);
}
