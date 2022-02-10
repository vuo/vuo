/**
 * @file
 * VuoWindowDescription implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Window Description",
					  "description" : "The settings for a window, such as its title and whether it is full-screen.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoWindowProperty"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @version200New
 */
VuoWindowDescription VuoWindowDescription_makeFromJson(json_object * js)
{
	return VuoList_VuoWindowProperty_makeFromJson(js);
}

/**
 * Encodes @c value as a JSON object.
 *
 * @version200New
 */
json_object * VuoWindowDescription_getJson(const VuoWindowDescription value)
{
	return VuoList_VuoWindowProperty_getJson(value);
}

/**
 * Returns a compact string representation of @c value.
 *
 * @version200New
 */
char * VuoWindowDescription_getSummary(const VuoWindowDescription value)
{
	return VuoList_VuoWindowProperty_getSummary(value);
}

/**
 * Returns a copy that can be modified without affecting the original.
 *
 * @version200New
 */
VuoWindowDescription VuoWindowDescription_copy(const VuoWindowDescription value)
{
	if (value)
		return (VuoWindowDescription)VuoListCopy_VuoWindowProperty((VuoList_VuoWindowProperty)value);

	return VuoListCreate_VuoWindowProperty();
}

/**
 * Changes a window setting.
 *
 * @version200New
 */
void VuoWindowDescription_setProperty(VuoWindowDescription value, VuoWindowProperty property)
{
	VuoListAppendValue_VuoWindowProperty((VuoList_VuoWindowProperty)value, property);
}

/**
 * Returns the settings stored in this window description in the format of a @ref VuoWindowProperty list.
 *
 * @version200New
 */
VuoList_VuoWindowProperty VuoWindowDescription_getWindowProperties(const VuoWindowDescription value)
{
	return (VuoList_VuoWindowProperty)value;
}
