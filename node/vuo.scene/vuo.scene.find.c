/**
 * @file
 * vuo.scene.find node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObjectType.h"

VuoModuleMetadata({
					 "title" : "Find 3D Objects",
					 "keywords" : [ "search", "hierarchy", "transform", "children", "child", "tree", "graph", "scene", "leaf" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

static bool typeMatches(VuoSceneObjectSubType subtype, VuoSceneObjectType type)
{
	switch(type)
	{
		case VuoSceneObjectType_Any:
			return true;

		case VuoSceneObjectType_Group:
			return subtype == VuoSceneObjectSubType_Group;
			// VuoSceneObjectSubType_Empty ??

		case VuoSceneObjectType_Mesh:
			return 	subtype == VuoSceneObjectSubType_Mesh;
				// subtype == VuoSceneObjectSubType_Text;

		case VuoSceneObjectType_Camera:
			return 	subtype == VuoSceneObjectSubType_PerspectiveCamera ||
					subtype == VuoSceneObjectSubType_StereoCamera ||
					subtype == VuoSceneObjectSubType_OrthographicCamera ||
					subtype == VuoSceneObjectSubType_FisheyeCamera;

		case VuoSceneObjectType_Light:
			return 	subtype == VuoSceneObjectSubType_AmbientLight ||
					subtype == VuoSceneObjectSubType_PointLight ||
					subtype == VuoSceneObjectSubType_Spotlight;
		default:
			return false;
	}
}

static void findSceneObjectsRecursive(const VuoSceneObject node, VuoText name, VuoSceneObjectType type, VuoList_VuoSceneObject foundObjects)
{
	if (typeMatches(VuoSceneObject_getType(node), type) &&
		(
			VuoText_isEmpty(name)
			|| (VuoSceneObject_getName(node) && strstr(VuoSceneObject_getName(node), name))
		)
	)
	{
		VuoListAppendValue_VuoSceneObject(foundObjects, node);
	}

	VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects(node);
	if (childObjects)
	{
		for (int i = 0; i < VuoListGetCount_VuoSceneObject(childObjects); i++)
		{
			VuoSceneObject child = VuoListGetValue_VuoSceneObject(childObjects, i+1);
			findSceneObjectsRecursive(child, name, type, foundObjects);
		}
	}
}

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoText) name,
	VuoInputData(VuoSceneObjectType, {"default":"any"}) type,
	VuoOutputData(VuoList_VuoSceneObject) foundObjects
)
{
	*foundObjects = VuoListCreate_VuoSceneObject();

	findSceneObjectsRecursive(object, name, type, *foundObjects);
}
