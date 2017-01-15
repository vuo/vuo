/**
 * @file
 * vuo.scene.fetch node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObjectGet.h"

VuoModuleMetadata({
					 "title" : "Fetch Scene",
					 "keywords" : [
						 "download", "open", "load", "import", "http", "url", "file", "get",
						 "mesh", "model", "object", "3D", "opengl", "scenegraph", "graphics",
						 "3ds", "Studio",
						 "Autodesk FBX",
						 "STL",
						 "Wavefront",
						 "Collada", "dae",
						 "Blender",
						 "dxf",
						 /* Stanford Polygon Library */ "ply",
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
					 "version" : "2.0.2",
					 "dependencies" : [
						 "VuoSceneObjectGet"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "DisplayScene.vuo" ]
					 }
				 });


void nodeEvent
(
		VuoInputData(VuoText, {"default":"", "name":"URL"}) url,
		VuoInputData(VuoBoolean, {"default":true}) center,
		VuoInputData(VuoBoolean, {"default":true}) fit,
		VuoInputData(VuoBoolean, {"name":"Has Left-handed Coordinates", "default":false}) hasLeftHandedCoordinates,
		VuoOutputData(VuoSceneObject) scene
)
{
	VuoSceneObject_get(url, scene, center, fit, hasLeftHandedCoordinates);
}
