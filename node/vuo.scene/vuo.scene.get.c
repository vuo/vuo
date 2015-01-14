/**
 * @file
 * vuo.scene.get node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoUrl.h"
#include "VuoGlContext.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#include <cimport.h>
#include <config.h>
#include <scene.h>
#include <postprocess.h>

VuoModuleMetadata({
					 "title" : "Get Scene",
					 "keywords" : [
						 "download", "open", "load", "import", "http",
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
						 "TrueSpace", "cob", "scn",
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
void vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(const struct aiScene *scene, const struct aiNode *node, VuoSceneObject *sceneObject)
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

		/// @todo handle materials
		sceneObject->shader = VuoShader_valueFromString("");
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

		/// @todo handle materials

		VuoListAppendValue_VuoVertices(sceneObject->verticesList, v);
	}

	if (node->mNumChildren)
		sceneObject->childObjects = VuoListCreate_VuoSceneObject();
	for (unsigned int child = 0; child < node->mNumChildren; ++child)
	{
		VuoSceneObject childSceneObject = VuoSceneObject_makeEmpty();
		vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(scene, node->mChildren[child], &childSceneObject);
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

	struct aiPropertyStore *props = aiCreatePropertyStore();
	VuoGlContext_use();
	{
		GLint maxIndices;
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &maxIndices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices/3);

		GLint maxVertices;
		glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices);
		aiSetImportPropertyInteger(props, AI_CONFIG_PP_SLM_VERTEX_LIMIT, maxVertices);
	}
	VuoGlContext_disuse();

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

	vuo_scene_get_convertAINodesToVuoSceneObjectsRecursively(ais, ais->mRootNode, scene);

	aiReleaseImport(ais);
}
