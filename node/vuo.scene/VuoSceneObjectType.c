/**
 * @file
 * VuoSceneObjectType implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSceneObjectType.h"
#include "VuoList_VuoSceneObjectType.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Scene Object Type",
					  "description" : "Scene object type.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoSceneObjectType"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoSceneObjectType
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "myTypeIs" : "group"
 *   }
 * }
 */
VuoSceneObjectType VuoSceneObjectType_makeFromJson(json_object * js)
{
	const char *valueAsString = "";
	if (json_object_get_type(js) == json_type_string)
		valueAsString = json_object_get_string(js);

	if (strcmp(valueAsString, "any") == 0)
		return VuoSceneObjectType_Any;
	else if (strcmp(valueAsString, "group") == 0)
		return VuoSceneObjectType_Group;
	else if (strcmp(valueAsString, "mesh") == 0)
		return VuoSceneObjectType_Mesh;
	else if (strcmp(valueAsString, "camera") == 0)
		return VuoSceneObjectType_Camera;
	else if (strcmp(valueAsString, "light") == 0)
		return VuoSceneObjectType_Light;

	return VuoSceneObjectType_Any;
}

/**
 * @ingroup VuoSceneObjectType
 * Encodes @c value as a JSON object.
 */
json_object * VuoSceneObjectType_getJson(const VuoSceneObjectType value)
{
	char *valueAsString = "any";

	if (value == VuoSceneObjectType_Any)
		valueAsString = "any";
	else if (value == VuoSceneObjectType_Group)
		valueAsString = "group";
	else if (value == VuoSceneObjectType_Mesh)
		valueAsString = "mesh";
	else if (value == VuoSceneObjectType_Camera)
		valueAsString = "camera";
	else if (value == VuoSceneObjectType_Light)
		valueAsString = "light";

	return json_object_new_string(valueAsString);
}

VuoSceneObjectType VuoSceneObjectType_makeFromSubtype(VuoSceneObjectSubType subType)
{
	switch(subType)
	{
	 	case VuoSceneObjectSubType_Empty:
	 	case VuoSceneObjectSubType_Group:
	 		return VuoSceneObjectType_Group;

	 	case VuoSceneObjectSubType_Mesh:
	 	case VuoSceneObjectSubType_Text:
	 		return VuoSceneObjectType_Mesh;

	 	case VuoSceneObjectSubType_PerspectiveCamera:
	 	case VuoSceneObjectSubType_StereoCamera:
	 	case VuoSceneObjectSubType_OrthographicCamera:
	 	case VuoSceneObjectSubType_FisheyeCamera:
	 		return VuoSceneObjectType_Camera;

	 	case VuoSceneObjectSubType_AmbientLight:
	 	case VuoSceneObjectSubType_PointLight:
	 	case VuoSceneObjectSubType_Spotlight:
	 		return VuoSceneObjectType_Light;

	 	default:
	 		return VuoSceneObjectType_Group;
	}
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoSceneObjectType VuoSceneObjectType_getAllowedValues(void)
{
	VuoList_VuoSceneObjectType l = VuoListCreate_VuoSceneObjectType();
	VuoListAppendValue_VuoSceneObjectType(l, VuoSceneObjectType_Any);
	VuoListAppendValue_VuoSceneObjectType(l, VuoSceneObjectType_Group);
	VuoListAppendValue_VuoSceneObjectType(l, VuoSceneObjectType_Mesh);
	VuoListAppendValue_VuoSceneObjectType(l, VuoSceneObjectType_Camera);
	VuoListAppendValue_VuoSceneObjectType(l, VuoSceneObjectType_Light);

	return l;
}

/**
 * @ingroup VuoSceneObjectType
 * Returns a compact string representation of @c value.
 */
char * VuoSceneObjectType_getSummary(const VuoSceneObjectType value)
{
	char *valueAsString = "Any";

	if (value == VuoSceneObjectType_Any)
		valueAsString = "Any";
	else if (value == VuoSceneObjectType_Group)
		valueAsString = "Group";
	else if (value == VuoSceneObjectType_Mesh)
		valueAsString = "Mesh";
	else if (value == VuoSceneObjectType_Camera)
		valueAsString = "Camera";
	else if (value == VuoSceneObjectType_Light)
		valueAsString = "Light";

	return strdup(valueAsString);
}
