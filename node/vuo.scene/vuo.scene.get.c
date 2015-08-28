/**
 * @file
 * vuo.scene.get node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoUrl.h"
#include "VuoGlContext.h"
#include "VuoImageGet.h"

#include "VuoPoint3d.h"	// todo Remove

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>

#include <cimport.h>
#include <config.h>
#include <scene.h>
#include <postprocess.h>

VuoModuleMetadata({
					 "title" : "Get Scene",
					 "keywords" : [
						 "download", "open", "load", "import", "http", "url", "file",
						 "mesh", "model", "object", "3D", "opengl", "scenegraph", "graphics",
						 "3ds", "Studio",
						 "Wavefront",
						 "Collada", "dae",
						 "Blender",
						 "dxf",
						 "Lightwave", "lwo",
						 "Modo", "lxo",
						 "DirectX",
						 "AC3D",
						 "Milkshape 3D", "ms3d",
						 /* TrueSpace */ "cob", "scn",
						 "Ogre", "xml",
						 "Irrlicht", "irrmesh",
						 "Quake", "Doom", "mdl", "md2", "md3", "pk3", "mdc", "md5",
						 "Starcraft", "m3",
						 "Valve", "smd",
						 "Unreal",
						 "Terragen", "terrain",
						 "PovRAY", "raw",
						 "BlitzBasic", "b3d",
						 "Quick3D", "q3d", "q3s",
						 "Neutral", "Sense8", "WorldToolKit", "format", "nff",
						 "off",
						 "GameStudio", "3DGS", "hmp",
						 "Izware", "Nendo", "ndo"
					 ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageGet",
						 "VuoUrl",
						 "assimp",
						 "z"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayScene.vuo" ]
					 }
				 });


/**
 * Converts @c node and its children to @c VuoSceneObjects.
 */
void vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(const struct aiScene *scene, const struct aiNode *node, VuoList_VuoShader shaders, VuoSceneObject *sceneObject)
{
	// Copy the node's transform into our VuoSceneObject.
	{
		struct aiMatrix4x4 m = node->mTransformation;
		struct aiVector3D scaling;
		struct aiQuaternion rotation;
		struct aiVector3D position;
		aiDecomposeMatrix(&m, &scaling, &rotation, &position);
		sceneObject->transform = VuoTransform_makeQuaternion(
					VuoPoint3d_make(position.x, position.y, position.z),
					VuoPoint4d_make(rotation.x, rotation.y, rotation.z, rotation.w),
					VuoPoint3d_make(scaling.x, scaling.y, scaling.z)
					);
	}

	// Convert each mesh to a VuoSubmesh instance.
	if (node->mNumMeshes)
	{
		sceneObject->mesh = VuoMesh_make(node->mNumMeshes);

			/// @todo Can a single aiNode use multiple aiMaterials?  If so, we need to split the aiNode into multiple VuoSceneObjects.  For now, just use the first mesh's material.
		int materialIndex = scene->mMeshes[node->mMeshes[0]]->mMaterialIndex;
		sceneObject->shader = VuoListGetValueAtIndex_VuoShader(shaders, materialIndex+1);
		VuoRetain(sceneObject->shader);
	}
	for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
	{
		const struct aiMesh *meshObj = scene->mMeshes[node->mMeshes[meshIndex]];

		if (!meshObj->mVertices)
		{
			VLog("Error: Mesh '%s' doesn't contain any positions.  Skipping.", meshObj->mName.data);
			continue;
		}

		VuoSubmesh sm = VuoSubmesh_make(meshObj->mNumVertices, meshObj->mNumFaces*3);
		sm.elementAssemblyMethod = VuoMesh_IndividualTriangles;

		for (unsigned int vertex = 0; vertex < meshObj->mNumVertices; ++vertex)
		{
			struct aiVector3D position = meshObj->mVertices[vertex];
			sm.positions[vertex] = VuoPoint4d_make(position.x, position.y, position.z, 1);

			if (meshObj->mNormals)
			{
				struct aiVector3D normal = meshObj->mNormals[vertex];
				sm.normals[vertex] = VuoPoint4d_make(normal.x, normal.y, normal.z, 0);
			}

			if (meshObj->mTangents)
			{
				struct aiVector3D tangent = meshObj->mTangents[vertex];
				sm.tangents[vertex] = VuoPoint4d_make(tangent.x, tangent.y, tangent.z, 0);
			}

			if (meshObj->mBitangents)
			{
				struct aiVector3D bitangent = meshObj->mBitangents[vertex];
				sm.bitangents[vertex] = VuoPoint4d_make(bitangent.x, bitangent.y, bitangent.z, 0);
			}

			if (meshObj->mTextureCoords[0])
			{
				struct aiVector3D textureCoordinate = meshObj->mTextureCoords[0][vertex];
				sm.textureCoordinates[vertex] = VuoPoint4d_make(textureCoordinate.x, textureCoordinate.y, textureCoordinate.z, 0);
			}

				/// @todo handle other texture coordinate channels
		}

		unsigned int numValidElements = 0;
		unsigned int minIndex=90000,maxIndex=0;
		for (unsigned int face = 0; face < meshObj->mNumFaces; ++face)
		{
			const struct aiFace *faceObj = &meshObj->mFaces[face];
			if (faceObj->mNumIndices != 3)
			{
				VLog("Warning: Face %u isn't a triangle (it has %u indices); skipping.",face,faceObj->mNumIndices);
				continue;
			}

			sm.elements[numValidElements++] = faceObj->mIndices[0];
			sm.elements[numValidElements++] = faceObj->mIndices[1];
			sm.elements[numValidElements++] = faceObj->mIndices[2];
			for (int i=0;i<3;++i)
			{
				if (faceObj->mIndices[i]<minIndex)
					minIndex=faceObj->mIndices[i];
				if (faceObj->mIndices[i]>maxIndex)
					maxIndex=faceObj->mIndices[i];
			}
		}

		sceneObject->mesh->submeshes[meshIndex] = sm;
	}

	VuoMesh_upload(sceneObject->mesh);

	if (node->mNumChildren)
		sceneObject->childObjects = VuoListCreate_VuoSceneObject();
	for (unsigned int child = 0; child < node->mNumChildren; ++child)
	{
		VuoSceneObject childSceneObject = VuoSceneObject_makeEmpty();
		vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(scene, node->mChildren[child], shaders, &childSceneObject);
		VuoListAppendValue_VuoSceneObject(sceneObject->childObjects, childSceneObject);
	}
}

void nodeEvent
(
		VuoInputData(VuoText, {"default":""}) sceneURL,
		VuoInputData(VuoBoolean, {"default":true}) center,
		VuoInputData(VuoBoolean, {"default":true}) fit,
		VuoInputData(VuoBoolean, {"default":false}) hasLeftHandedCoordinates,
		VuoOutputData(VuoSceneObject) scene
)
{
	*scene = VuoSceneObject_makeEmpty();

	if (!strlen(sceneURL))
		return;

	void *data;
	unsigned int dataLength;
	if (!VuoUrl_get(sceneURL, &data, &dataLength))
	{
		VLog("Error: Didn't get any scene data for '%s'.", sceneURL);
		return;
	}

	CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
	struct aiPropertyStore *props = aiCreatePropertyStore();
	{
		GLint maxIndices;
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxIndices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices/3);

		GLint maxVertices;
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_VERTEX_LIMIT, maxVertices);
	}

	const struct aiScene *ais = aiImportFileFromMemoryWithProperties(
				data,
				dataLength,
				aiProcess_Triangulate
//				| aiProcess_PreTransformVertices
				| aiProcess_CalcTangentSpace
				| aiProcess_GenSmoothNormals
				| aiProcess_SplitLargeMeshes
				| aiProcess_GenUVCoords,
//				| aiProcess_OptimizeMeshes
//				| aiProcess_OptimizeGraph
				"",
				props);
	aiReleasePropertyStore(props);
	if (!ais)
	{
		VLog("Error reading '%s': %s\n", sceneURL, aiGetErrorString());
		return;
	}

	VuoText normalizedSceneURL = VuoUrl_normalize(sceneURL);
	VuoRetain(normalizedSceneURL);
	size_t lastSlashInSceneURL = VuoText_findLastOccurrence(normalizedSceneURL, "/");
	VuoText sceneURLWithoutFilename = VuoText_substring(normalizedSceneURL, 1, lastSlashInSceneURL);
	VuoRetain(sceneURLWithoutFilename);
	VuoRelease(normalizedSceneURL);

	VuoList_VuoShader shaders = VuoListCreate_VuoShader();
	VuoRetain(shaders);

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
//		VLog("\tdiffuseColor: %s",VuoColor_summaryFromValue(diffuseColor));

		struct aiColor4D specularColorAI = {1,1,1,1};
		aiGetMaterialColor(m, AI_MATKEY_COLOR_SPECULAR, &specularColorAI);
		VuoColor specularColor = VuoColor_makeWithRGBA(specularColorAI.r, specularColorAI.g, specularColorAI.b, specularColorAI.a);
	//		VLog("\tspecularColor: %s",VuoColor_summaryFromValue(specularColor));

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
			VLog("\tambient: %s",path.data);
		}

		int emissiveTextures = aiGetMaterialTextureCount(m, aiTextureType_EMISSIVE);
		if (emissiveTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_EMISSIVE, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\temissive: %s",path.data);
		}

		int opacityTextures = aiGetMaterialTextureCount(m, aiTextureType_OPACITY);
		if (opacityTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_OPACITY, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\topacity: %s",path.data);
		}

		int lightmapTextures = aiGetMaterialTextureCount(m, aiTextureType_LIGHTMAP);
		if (lightmapTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_LIGHTMAP, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\tlightmap: %s",path.data);
		}

		int reflectionTextures = aiGetMaterialTextureCount(m, aiTextureType_REFLECTION);
		if (reflectionTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_REFLECTION, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\treflection: %s",path.data);
		}

		int displacementTextures = aiGetMaterialTextureCount(m, aiTextureType_DISPLACEMENT);
		if (displacementTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_DISPLACEMENT, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\tdisplacement: %s",path.data);
		}

		int heightTextures = aiGetMaterialTextureCount(m, aiTextureType_HEIGHT);
		if (heightTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_HEIGHT, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\theight: %s",path.data);
		}

		int unknownTextures = aiGetMaterialTextureCount(m, aiTextureType_UNKNOWN);
		if (unknownTextures)
		{
			struct aiString path;
			aiGetMaterialTexture(m, aiTextureType_UNKNOWN, 0, &path, NULL, NULL, NULL, NULL, NULL, NULL);
			VLog("\tunknown: %s",path.data);
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

		shader->name = strdup(name.data);

		// VLog("\tshader: %s",shader->summary);//VuoShader_summaryFromValue(shader));

		VuoListAppendValue_VuoShader(shaders, shader);
	}
	VuoRelease(sceneURLWithoutFilename);
	VuoGlContext_disuse(cgl_ctx);

	vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(ais, ais->mRootNode, shaders, scene);
	VuoRelease(shaders);

	if(center)
		VuoSceneObject_center(scene);

	if(fit)
		VuoSceneObject_normalize(scene);

	if(hasLeftHandedCoordinates)
	{
		VuoSceneObject_apply(scene, ^(VuoSceneObject *currentObject, float modelviewMatrix[16])
		{
			// VuoTransform flipAxis = VuoTransform_makeEuler( (VuoPoint3d) {0,0,0}, (VuoPoint3d){0,0,0}, (VuoPoint3d){-1, 1, 1} );
			// float matrix[16];
			// VuoTransform_getMatrix(flipAxis, matrix);

			if(currentObject->mesh != NULL)
			{
				for(int i = 0; i < currentObject->mesh->submeshCount; i++)
				{
					VuoSubmesh msh = currentObject->mesh->submeshes[i];
					for(int n = 0; n < msh.vertexCount; n++)
					{
						msh.positions[n].x *= -1;
					}

					// flip triangle winding order
					switch(msh.elementAssemblyMethod)
					{
						case VuoMesh_IndividualTriangles:
						{
							unsigned int elementCount = msh.elementCount;
							for(int i = 0; i < elementCount; i+= 3)
							{
								unsigned int tmp = msh.elements[i];
								msh.elements[i] = msh.elements[i+2];
								msh.elements[i+2] = tmp;
							}
						} break;

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
				VuoMesh_upload(currentObject->mesh);
			}
	});
	}

	aiReleaseImport(ais);
}
