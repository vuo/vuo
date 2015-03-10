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
					 "version" : "1.0.0",
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
					VuoPoint4d_make(rotation.w, rotation.x, rotation.y, rotation.z),
					VuoPoint3d_make(scaling.x, scaling.y, scaling.z)
					);
	}

	// Convert each mesh to a VuoVertices instance.
	if (node->mNumMeshes)
	{
		sceneObject->verticesList = VuoListCreate_VuoVertices();

		/// @todo Can a single aiNode use multiple aiMaterials?  If so, we need to split the aiNode into multiple VuoSceneObjects.  For now, just use the first mesh's material.
		int materialIndex = scene->mMeshes[node->mMeshes[0]]->mMaterialIndex;
		sceneObject->shader = VuoListGetValueAtIndex_VuoShader(shaders, materialIndex+1);
		VuoRetain(sceneObject->shader);
	}
	for (unsigned int mesh = 0; mesh < node->mNumMeshes; ++mesh)
	{
		const struct aiMesh *meshObj = scene->mMeshes[node->mMeshes[mesh]];

		if (!meshObj->mVertices)
		{
			fprintf(stderr, "vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively() Error: Mesh '%s' doesn't contain any positions.  Skipping.\n", meshObj->mName.data);
			continue;
		}

		VuoVertices v = VuoVertices_alloc(meshObj->mNumVertices, meshObj->mNumFaces*3);
		v.elementAssemblyMethod = VuoVertices_IndividualTriangles;

		for (unsigned int vertex = 0; vertex < meshObj->mNumVertices; ++vertex)
		{
			struct aiVector3D position = meshObj->mVertices[vertex];
			v.positions[vertex] = VuoPoint4d_make(position.x, position.y, position.z, 1);

			if (meshObj->mNormals)
			{
				struct aiVector3D normal = meshObj->mNormals[vertex];
				v.normals[vertex] = VuoPoint4d_make(normal.x, normal.y, normal.z, 0);
			}

			if (meshObj->mTangents)
			{
				struct aiVector3D tangent = meshObj->mTangents[vertex];
				v.tangents[vertex] = VuoPoint4d_make(tangent.x, tangent.y, tangent.z, 0);
			}

			if (meshObj->mBitangents)
			{
				struct aiVector3D bitangent = meshObj->mBitangents[vertex];
				v.bitangents[vertex] = VuoPoint4d_make(bitangent.x, bitangent.y, bitangent.z, 0);
			}

			if (meshObj->mTextureCoords[0])
			{
				struct aiVector3D textureCoordinate = meshObj->mTextureCoords[0][vertex];
				v.textureCoordinates[vertex] = VuoPoint4d_make(textureCoordinate.x, textureCoordinate.y, textureCoordinate.z, 0);
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
				fprintf(stderr, "vuo.scene.get Warning: Face %d isn't a triangle (it has %d indices); skipping.\n",face,faceObj->mNumIndices);
				continue;
			}

			v.elements[numValidElements++] = faceObj->mIndices[0];
			v.elements[numValidElements++] = faceObj->mIndices[1];
			v.elements[numValidElements++] = faceObj->mIndices[2];
			for (int i=0;i<3;++i)
			{
				if (faceObj->mIndices[i]<minIndex)
					minIndex=faceObj->mIndices[i];
				if (faceObj->mIndices[i]>maxIndex)
					maxIndex=faceObj->mIndices[i];
			}
		}

		VuoListAppendValue_VuoVertices(sceneObject->verticesList, v);
	}

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
		fprintf(stderr, "vuo.scene.get Error: Didn't get any scene data for '%s'.\n", sceneURL);
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
		fprintf(stderr, "vuo.scene.get Error: %s\n", aiGetErrorString());
		return;
	}

	char *normalizedSceneURL = VuoUrl_normalize(sceneURL);
	size_t lastSlashInSceneURL = VuoText_getIndexOfLastCharacter(normalizedSceneURL, "/");
	VuoText sceneURLWithoutFilename = VuoText_substring(normalizedSceneURL, 1, lastSlashInSceneURL);
	VuoRetain(sceneURLWithoutFilename);

	VuoList_VuoShader shaders = VuoListCreate_VuoShader();
	VuoRetain(shaders);

	for (int i=0; i<ais->mNumMaterials; ++i)
	{
		struct aiMaterial *m = ais->mMaterials[i];

//		struct aiString name;
//		aiGetMaterialString(m, AI_MATKEY_NAME, &name);
//		VLog("material %d: %s",i,name.data);

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

			VuoImage specularImage = VuoImage_get(textureURL);

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

			shader = VuoShader_makeLitImageDetailsShader(diffuseImage, 1, normalImage, specularImage);
		}
		else if (diffuseImage)
			shader = VuoShader_makeLitImageShader(diffuseImage, 1, specularColor, shininess);
		else
			shader = VuoShader_makeLitColorShader(diffuseColor, specularColor, shininess);

//		VLog("\tshader: %s",VuoShader_summaryFromValue(shader));

		VuoListAppendValue_VuoShader(shaders, shader);
	}
	VuoRelease(sceneURLWithoutFilename);
	VuoGlContext_disuse(cgl_ctx);

	vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(ais, ais->mRootNode, shaders, scene);
	VuoRelease(shaders);

	aiReleaseImport(ais);
}
