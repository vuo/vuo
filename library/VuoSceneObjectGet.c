/**
 * @file
 * VuoSceneGet implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectGet.h"

#include "VuoUrl.h"
#include "VuoUrlFetch.h"
#include "VuoImageGet.h"
#include "VuoMeshUtility.h"

#include <OpenGL/CGLMacro.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
	#include <cimport.h>
	#include <config.h>
	#include <scene.h>
	#include <postprocess.h>

	// cfileio.h won't compile without these.
	#ifndef DOXYGEN
		typedef enum aiOrigin aiOrigin;
		typedef struct aiFile aiFile;
		typedef struct aiFileIO aiFileIO;
	#endif
	#include <cfileio.h>
#pragma clang diagnostic pop

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoSceneGet",
					  "dependencies" : [
							"VuoSceneObject",
							"VuoGlContext",
							"VuoImageGet",
							"VuoShader",
							"VuoUrlFetch",
							"VuoList_VuoShader",
							"VuoMeshUtility",
							"oai",
							"IrrXML",
							"z"
					  ]
				  });
#endif



/**
 * A data buffer, and the curren read position inside it.
 */
typedef struct
{
	size_t dataLength;
	void *data;
	size_t position;
} VuoSceneObjectGet_data;

/**
 * Reads bytes from VuoSceneObjectGet_data.
 */
static size_t VuoSceneObjectGet_read(aiFile *af, char *buffer, size_t size, size_t count)
{
//	VLog("	%ld (%ld * %ld)", size*count, size, count);
	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)af->UserData;
	if (d->position >= d->dataLength)
	{
//		VLog("	refusing to read past end");
		return 0;
	}

	size_t bytesToRead = MIN(d->dataLength - d->position, size*count);
	memcpy(buffer, d->data + d->position, bytesToRead);
//	VLog("	actually read %ld", bytesToRead);
	d->position += bytesToRead;
	return bytesToRead;
}

/**
 * Gives VuoSceneObjectGet_data's current position.
 */
static size_t VuoSceneObjectGet_tell(aiFile *af)
{
	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)af->UserData;
//	VLog("	position = %ld", d->position);
	return d->position;
}

/**
 * Gives the size of VuoSceneObjectGet_data's data.
 */
static size_t VuoSceneObjectGet_size(aiFile *af)
{
	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)af->UserData;
	return d->dataLength;
}

/**
 * Moves VuoSceneObjectGet_data's position.
 */
static aiReturn VuoSceneObjectGet_seek(aiFile *af, size_t p, aiOrigin origin)
{
//	VLog("	seek by %ld (mode %d)", p, origin);
	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)af->UserData;

	size_t proposedPosition;
	if (origin == aiOrigin_SET)
		proposedPosition = p;
	else if (origin == aiOrigin_CUR)
		proposedPosition = d->position + p;
	else if (origin == aiOrigin_END)
		proposedPosition = d->dataLength - p;
	else
		return aiReturn_FAILURE;

	if (proposedPosition >= d->dataLength)
		return aiReturn_FAILURE;

	d->position = proposedPosition;
	return aiReturn_SUCCESS;
}

/**
 * Reads a file into a data buffer, and provides functions for accessing it.
 */
static aiFile *VuoSceneObjectGet_open(aiFileIO *afio, const char *filename, const char *mode)
{
//	VLog("%s", filename);

	if (strcmp(mode, "rb") != 0)
	{
		VUserLog("Error: Unknown file mode '%s'",mode);
		return NULL;
	}

	void *data;
	unsigned int dataLength;
	VuoText filenameT = VuoText_make(filename);
	VuoLocal(filenameT);
	if (!VuoUrl_fetch(filenameT, &data, &dataLength))
		return NULL;

	if (dataLength == 0)
	{
		free(data);
		VUserLog("Warning: '%s' is empty", filename);
		return NULL;
	}

	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)malloc(sizeof(VuoSceneObjectGet_data));
	d->data = data;
	d->dataLength = dataLength;
	d->position = 0;

	aiFile *af = (aiFile *)malloc(sizeof(aiFile));
	af->UserData = (aiUserData)d;

	af->ReadProc     = VuoSceneObjectGet_read;
	af->TellProc     = VuoSceneObjectGet_tell;
	af->FileSizeProc = VuoSceneObjectGet_size;
	af->SeekProc     = VuoSceneObjectGet_seek;
	af->WriteProc    = NULL;
	af->FlushProc    = NULL;

	return af;
}

/**
 * Frees the data buffer.
 */
static void VuoSceneObjectGet_close(aiFileIO *afio, aiFile *af)
{
	VuoSceneObjectGet_data *d = (VuoSceneObjectGet_data *)af->UserData;
	free(d->data);
	free(af);
}



/**
 * Converts @c node and its children to @c VuoSceneObjects.
 */
static void convertAINodesToVuoSceneObjectsRecursively(const struct aiScene *scene, const struct aiNode *node, VuoShader *shaders, bool *shadersUsed, VuoSceneObject *sceneObject)
{
	VuoSceneObject_setName(*sceneObject, VuoText_make(node->mName.data));

	// Copy the node's transform into our VuoSceneObject.
	{
		struct aiMatrix4x4 m = node->mTransformation;
		struct aiVector3D scaling;
		struct aiQuaternion rotation;
		struct aiVector3D position;
		aiDecomposeMatrix(&m, &scaling, &rotation, &position);
		VuoSceneObject_setTransform(*sceneObject, VuoTransform_makeQuaternion(
			(VuoPoint3d){position.x, position.y, position.z},
			(VuoPoint4d){rotation.x, rotation.y, rotation.z, rotation.w},
			(VuoPoint3d){scaling.x, scaling.y, scaling.z}));
	}

	// Convert each aiMesh to either a single leaf VuoSceneObject,
	// or a VuoSceneObject group with multiple child VuoSceneObjects.
	if (node->mNumMeshes && node->mNumMeshes == 1 && node->mNumChildren == 0)
		VuoSceneObject_setType(*sceneObject, VuoSceneObjectSubType_Mesh);
	else
	{
		VuoSceneObject_setType(*sceneObject, VuoSceneObjectSubType_Group);
		VuoSceneObject_setChildObjects(*sceneObject, VuoListCreate_VuoSceneObject());
	}
	for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
	{
		const struct aiMesh *meshObj = scene->mMeshes[node->mMeshes[meshIndex]];

		if (!meshObj->mVertices)
		{
			VUserLog("Error: Mesh '%s' doesn't contain any positions.  Skipping.", meshObj->mName.data);
			continue;
		}

		if (!meshObj->mNormals || !meshObj->mTextureCoords[0])
			VUserLog("Warning: Mesh '%s' is missing%s%s.  These channels will be automatically generated, but lighting and 3D object filters may not work correctly.",
				meshObj->mName.data,
				meshObj->mNormals ? "" : " [normals]",
				meshObj->mTextureCoords[0] ? "" : " [texture coordinates]");

		float *positions, *normals = NULL, *textureCoordinates = NULL, *colors = NULL;
		unsigned int *elements;
		VuoMesh_allocateCPUBuffers(meshObj->mNumVertices,
			&positions,
			meshObj->mNormals ? &normals : NULL,
			meshObj->mTextureCoords[0] ? &textureCoordinates : NULL,
			meshObj->mColors[0] ? &colors : NULL,
			meshObj->mNumFaces * 3, &elements);

		for (unsigned int vertex = 0; vertex < meshObj->mNumVertices; ++vertex)
		{
			struct aiVector3D position = meshObj->mVertices[vertex];
			positions[vertex * 3    ] = position.x;
			positions[vertex * 3 + 1] = position.y;
			positions[vertex * 3 + 2] = position.z;

			if (meshObj->mNormals)
			{
				struct aiVector3D normal = meshObj->mNormals[vertex];
				normals[vertex * 3    ] = normal.x;
				normals[vertex * 3 + 1] = normal.y;
				normals[vertex * 3 + 2] = normal.z;
			}

			if (meshObj->mTextureCoords[0])
			{
				struct aiVector3D textureCoordinate = meshObj->mTextureCoords[0][vertex];
				textureCoordinates[vertex * 2    ] = textureCoordinate.x;
				textureCoordinates[vertex * 2 + 1] = textureCoordinate.y;
			}

			if (meshObj->mColors[0])
			{
				struct aiColor4D color = meshObj->mColors[0][vertex];
				colors[vertex * 4    ] = color.r;
				colors[vertex * 4 + 1] = color.g;
				colors[vertex * 4 + 2] = color.b;
				colors[vertex * 4 + 3] = color.a;
			}
		}

		unsigned int numValidElements = 0;
		for (unsigned int face = 0; face < meshObj->mNumFaces; ++face)
		{
			const struct aiFace *faceObj = &meshObj->mFaces[face];
			if (faceObj->mNumIndices != 3)
			{
				VUserLog("Warning: Face %u isn't a triangle (it has %u indices); skipping.",face,faceObj->mNumIndices);
				continue;
			}

			elements[numValidElements++] = faceObj->mIndices[0];
			elements[numValidElements++] = faceObj->mIndices[1];
			elements[numValidElements++] = faceObj->mIndices[2];
		}

		VuoMesh mesh = VuoMesh_makeFromCPUBuffers(meshObj->mNumVertices, positions, normals, textureCoordinates, colors, numValidElements, elements, VuoMesh_IndividualTriangles);

		// if no texture coordinates found, attempt to generate passable ones.
		if (!meshObj->mTextureCoords[0])
			VuoMeshUtility_calculateSphericalUVs(mesh);

		if (node->mNumMeshes == 1 && node->mNumChildren == 0)
		{
			VuoSceneObject_setMesh(*sceneObject, mesh);
			VuoSceneObject_setShader(*sceneObject, shaders[meshObj->mMaterialIndex]);
			shadersUsed[meshObj->mMaterialIndex] = true;
		}
		else
		{
			// Add this aiMesh as a child of this VuoSceneObject.
			VuoSceneObject child = VuoSceneObject_makeMesh(mesh, shaders[meshObj->mMaterialIndex], VuoTransform_makeIdentity());
			VuoListAppendValue_VuoSceneObject(VuoSceneObject_getChildObjects(*sceneObject), child);
			shadersUsed[meshObj->mMaterialIndex] = true;
		}
	}

	for (unsigned int child = 0; child < node->mNumChildren; ++child)
	{
		VuoSceneObject childSceneObject = VuoSceneObject_makeEmpty();
		convertAINodesToVuoSceneObjectsRecursively(scene, node->mChildren[child], shaders, shadersUsed, &childSceneObject);
		VuoListAppendValue_VuoSceneObject(VuoSceneObject_getChildObjects(*sceneObject), childSceneObject);
	}
}

/**
 * @ingroup VuoSceneObject
 * Retrieves the scene at the specified @a sceneURL, creates a @c VuoSceneObject from it, and stores it in @a scene.
 *
 * Returns false if the scene could not be loaded.
 */
bool VuoSceneObject_get(VuoText sceneURL, VuoSceneObject *scene, bool center, bool fit, bool hasLeftHandedCoordinates)
{
	*scene = NULL;

	if (VuoText_isEmpty(sceneURL))
		return false;

	struct aiPropertyStore *props = aiCreatePropertyStore();
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
		GLint maxIndices;
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxIndices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices/3);

		GLint maxVertices;
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_VERTEX_LIMIT, maxVertices);

		static bool limitsLogged = false;
		if (!limitsLogged)
		{
			limitsLogged = true;
			VDebugLog("OpenGL driver reports maxIndices=%d, maxVertices=%d", maxIndices, maxVertices);
		}
	});

	struct aiFileIO fileHandlers;
	fileHandlers.OpenProc = VuoSceneObjectGet_open;
	fileHandlers.CloseProc = VuoSceneObjectGet_close;

	const struct aiScene *ais = aiImportFileExWithProperties(
				sceneURL,
				aiProcess_Triangulate
//				| aiProcess_PreTransformVertices
//				| aiProcess_CalcTangentSpace
				| aiProcess_GenSmoothNormals
				| aiProcess_SplitLargeMeshes
				| aiProcess_GenUVCoords,
//				| aiProcess_OptimizeMeshes
//				| aiProcess_OptimizeGraph
				&fileHandlers,
				props);
	aiReleasePropertyStore(props);
	if (!ais)
	{
		VUserLog("Error: %s", aiGetErrorString());
		return false;
	}

	if (ais->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
		VUserLog("Warning: Open Asset Import wasn't able to parse everything in this file.");

	VuoText normalizedSceneURL = VuoUrl_normalize(sceneURL, VuoUrlNormalize_default);
	VuoRetain(normalizedSceneURL);
	size_t lastSlashInSceneURL = VuoText_findLastOccurrence(normalizedSceneURL, "/");
	VuoText sceneURLWithoutFilename = VuoText_substring(normalizedSceneURL, 1, lastSlashInSceneURL);
	VuoRetain(sceneURLWithoutFilename);
	VuoRelease(normalizedSceneURL);

	VuoShader shaders[ais->mNumMaterials];
	bool shadersUsed[ais->mNumMaterials];
	for (int i=0; i<ais->mNumMaterials; ++i)
	{
		struct aiMaterial *m = ais->mMaterials[i];

		struct aiString name;
		aiGetMaterialString(m, AI_MATKEY_NAME, &name);
		// VLog("material %d: %s",i,name.data);

		// Some meshes (such as the leaves in "Tree 2 N020414.3DS") obviously expect two-sided rendering,
		// so I tried using material property AI_MATKEY_TWOSIDED.  But I wasn't able to find (or create with Blender)
		// any meshes having materials where AI_MATKEY_TWOSIDED is nonzero.
//		int twosided = 0;
//		aiGetMaterialIntegerArray(m, AI_MATKEY_TWOSIDED, &twosided, NULL);

		VuoShader shader = NULL;

		struct aiColor4D diffuseColorAI = {1,1,1,1};
		aiGetMaterialColor(m, AI_MATKEY_COLOR_DIFFUSE, &diffuseColorAI);
		VuoColor diffuseColor = VuoColor_makeWithRGBA(diffuseColorAI.r, diffuseColorAI.g, diffuseColorAI.b, diffuseColorAI.a);
//		VLog("\tdiffuseColor: %s",VuoColor_getSummary(diffuseColor));

		struct aiColor4D specularColorAI = {1,1,1,1};
		aiGetMaterialColor(m, AI_MATKEY_COLOR_SPECULAR, &specularColorAI);
		VuoColor specularColor = VuoColor_makeWithRGBA(specularColorAI.r, specularColorAI.g, specularColorAI.b, specularColorAI.a);
	//		VLog("\tspecularColor: %s",VuoColor_getSummary(specularColor));

		float shininess = 10;
		aiGetMaterialFloatArray(m, AI_MATKEY_SHININESS, &shininess, NULL);
		if (shininess)
			shininess = MAX(1.0001 - 1./shininess, 0);
	//		VLog("\tshininess: %g",shininess);

		int diffuseTextures = aiGetMaterialTextureCount(m, aiTextureType_DIFFUSE);
		VuoImage diffuseImage = NULL;
			/// @todo load and blend multiple diffuse textures
		if (diffuseTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			VuoText urlParts[2] = {sceneURLWithoutFilename, path.data};
			VuoText textureURL = VuoText_append(urlParts, 2);
			VuoRetain(textureURL);
//			VLog("\tdiffuse: %s",textureURL);

			diffuseImage = VuoImage_get(textureURL);

			VuoRelease(textureURL);
		}

		int normalTextures = aiGetMaterialTextureCount(m, aiTextureType_NORMALS);
		VuoImage normalImage = NULL;
		if (normalTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_NORMALS, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			VuoText urlParts[2] = {sceneURLWithoutFilename, path.data};
			VuoText textureURL = VuoText_append(urlParts, 2);
			VuoRetain(textureURL);
//			VLog("\tnormal: %s",textureURL);

			normalImage = VuoImage_get(textureURL);

			VuoRelease(textureURL);
		}

		int specularTextures = aiGetMaterialTextureCount(m, aiTextureType_SPECULAR);
		VuoImage specularImage = NULL;
		if (specularTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_SPECULAR, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);

			VuoText urlParts[2] = {sceneURLWithoutFilename, path.data};
			VuoText textureURL = VuoText_append(urlParts, 2);
			VuoRetain(textureURL);
//			VLog("\tspecular: %s",textureURL);

			specularImage = VuoImage_get(textureURL);

			VuoRelease(textureURL);
		}

// Other texture types to consider supporting eventually...
#if 0
		int ambientTextures = aiGetMaterialTextureCount(m, aiTextureType_AMBIENT);
		if (ambientTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_AMBIENT, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\tambient: %s",path.data);
		}

		int emissiveTextures = aiGetMaterialTextureCount(m, aiTextureType_EMISSIVE);
		if (emissiveTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_EMISSIVE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\temissive: %s",path.data);
		}

		int opacityTextures = aiGetMaterialTextureCount(m, aiTextureType_OPACITY);
		if (opacityTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_OPACITY, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\topacity: %s",path.data);
		}

		int lightmapTextures = aiGetMaterialTextureCount(m, aiTextureType_LIGHTMAP);
		if (lightmapTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_LIGHTMAP, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\tlightmap: %s",path.data);
		}

		int reflectionTextures = aiGetMaterialTextureCount(m, aiTextureType_REFLECTION);
		if (reflectionTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_REFLECTION, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\treflection: %s",path.data);
		}

		int displacementTextures = aiGetMaterialTextureCount(m, aiTextureType_DISPLACEMENT);
		if (displacementTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_DISPLACEMENT, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\tdisplacement: %s",path.data);
		}

		int heightTextures = aiGetMaterialTextureCount(m, aiTextureType_HEIGHT);
		if (heightTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_HEIGHT, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\theight: %s",path.data);
		}

		int unknownTextures = aiGetMaterialTextureCount(m, aiTextureType_UNKNOWN);
		if (unknownTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_UNKNOWN, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VUserLog("\tunknown: %s",path.data);
		}
#endif

		if (normalImage || specularImage)
		{
			if (!diffuseImage)
				diffuseImage = VuoImage_makeColorImage(diffuseColor, 1, 1);

			if (!normalImage)
				normalImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0.5,0.5,1,1), 1, 1);

			if (!specularImage)
				specularImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,.9), 1, 1);

			VuoImage_setWrapMode(diffuseImage, VuoImageWrapMode_Repeat);
			VuoImage_setWrapMode(normalImage, VuoImageWrapMode_Repeat);
			VuoImage_setWrapMode(specularImage, VuoImageWrapMode_Repeat);
			shader = VuoShader_makeLitImageDetailsShader(diffuseImage, 1, specularImage, normalImage);
		}
		else if (diffuseImage)
		{
			VuoImage_setWrapMode(diffuseImage, VuoImageWrapMode_Repeat);
			shader = VuoShader_makeLitImageShader(diffuseImage, 1, specularColor, shininess);
		}
		else
			shader = VuoShader_makeLitColorShader(diffuseColor, specularColor, shininess);

		VuoRelease(shader->name);
		shader->name = VuoText_make(name.data);
		VuoRetain(shader->name);

		// VLog("\tshader: %s",shader->summary);//VuoShader_getSummary(shader));

		shaders[i] = shader;
		shadersUsed[i] = false;
	}
	VuoRelease(sceneURLWithoutFilename);

	*scene = VuoSceneObject_makeEmpty();
	convertAINodesToVuoSceneObjectsRecursively(ais, ais->mRootNode, shaders, shadersUsed, scene);

	for (unsigned int i = 0; i < ais->mNumMaterials; ++i)
		if (!shadersUsed[i])
		{
			VuoRetain(shaders[i]);
			VuoRelease(shaders[i]);
		}

	if(center)
		VuoSceneObject_center(*scene);

	if(fit)
		VuoSceneObject_normalize(*scene);

	if(hasLeftHandedCoordinates)
	{
		VuoSceneObject_apply(*scene, ^(VuoSceneObject currentObject, float modelviewMatrix[16])
		{
			// VuoTransform flipAxis = VuoTransform_makeEuler( (VuoPoint3d) {0,0,0}, (VuoPoint3d){0,0,0}, (VuoPoint3d){-1, 1, 1} );
			// float matrix[16];
			// VuoTransform_getMatrix(flipAxis, matrix);

			VuoMesh mesh = VuoSceneObject_getMesh(currentObject);
			if (mesh)
			{
				unsigned int vertexCount, elementCount, *elements;
				float *positions;
				VuoMesh_getCPUBuffers(mesh, &vertexCount, &positions, NULL, NULL, NULL, &elementCount, &elements);

				for (int n = 0; n < vertexCount; n++)
					positions[n * 3] *= -1;

				// flip triangle winding order
				switch (VuoMesh_getElementAssemblyMethod(mesh))
				{
					case VuoMesh_IndividualTriangles:
						for(int i = 0; i < elementCount; i+= 3)
						{
							unsigned int tmp = elements[i];
							elements[i] = elements[i+2];
							elements[i+2] = tmp;
						}
						break;

						// @todo - fill in additional winding order flip methods whenever this loader
						// provides the ability to read 'em
						// http://www.asawicki.info/news_1524_how_to_flip_triangles_in_triangle_mesh.html
						// case VuoMesh_TriangleStrip:
						// {
						// 	unsigned int elementCount = msh.elementCount;

						// 	// add an extra duplicate vertex at the beginning of the strip
						// 	// if the element count is even
						// 	if(elementCount % 2 == 0)
						// 	{
						// 		msh.elementCount++;
						// 		unsigned int *resized = malloc(sizeof(unsigned int) * msh.elementCount);
						// 		resized[0] = msh.elements[0];
						// 		memcpy(&resized[1], msh.elements, sizeof(unsigned int) * msh.elementCount);
						// 		msh.elements = resized;
						// 	}
						// 	else
						// 	{
						// 		for(int i = 0; i < elementCount / 2; i++)
						// 		{
						// 			unsigned int tmp = msh.elements[i];
						// 			msh.elements[i] = msh.elements[elementCount-i];
						// 			msh.elements[i] = tmp;
						// 		}
						// 	}
						// } break;

						default:
							break;
				}
			}
		});
	}

	aiReleaseImport(ais);

	return true;
}
