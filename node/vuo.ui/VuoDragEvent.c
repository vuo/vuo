/**
 * @file
 * VuoDragEvent implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoDragEvent.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Drag Event",
					  "description" : "Information about an in-progress or completed file drag.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoPoint2d",
						  "VuoUrl",
						  "VuoList_VuoUrl"
					  ]
				  });
#endif
/// @}

/**
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "position": ["x":0, "y":0],
 *     "urls": ["file:///Users/me/Desktop/test.jpg"]
 *   }
 * }
 */
VuoDragEvent VuoDragEvent_makeFromJson(json_object * js)
{
	VuoDragEvent value = {{0,0}, NULL};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "position", &o))
		value.position = VuoPoint2d_makeFromJson(o);

	if (json_object_object_get_ex(js, "urls", &o))
		value.urls = VuoList_VuoUrl_makeFromJson(o);

	return value;
}

/**
 * Encodes @c value as a JSON object.
 */
json_object * VuoDragEvent_getJson(const VuoDragEvent value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "position", VuoPoint2d_getJson(value.position));

	if (value.urls)
		json_object_object_add(js, "urls", VuoList_VuoUrl_getJson(value.urls));

	return js;
}

/**
 * Returns a compact string representation of @c value.
 */
char *VuoDragEvent_getSummary(const VuoDragEvent value)
{
	char *listSummary = VuoList_VuoUrl_getSummary(value.urls);
	char *summary = VuoText_format("%g,%g<br>%s", value.position.x, value.position.y, listSummary);
	free(listSummary);
	return summary;
}

/**
 * Creates a new drag event.
 */
VuoDragEvent VuoDragEvent_make(const VuoPoint2d position, const VuoList_VuoUrl urls)
{
	return (VuoDragEvent){position, urls};
}

/**
 * Returns true if the two drag events are equivalent:
 *
 *    - The positions must match
 *    - The URLs must match
 */
bool VuoDragEvent_areEqual(const VuoDragEvent value1, const VuoDragEvent value2)
{
	if (!VuoPoint2d_areEqual(value1.position, value2.position))
		return false;

	if (!VuoList_VuoUrl_areEqual(value1.urls, value2.urls))
		return false;

	return true;
}

/**
 * Returns true if value1 < value2.
 */
bool VuoDragEvent_isLessThan(const VuoDragEvent a, const VuoDragEvent b)
{
	if (a.position.x < b.position.x) return true;
	if (a.position.x > b.position.x) return false;

	if (a.position.y < b.position.y) return true;
	if (a.position.y > b.position.y) return false;

	if (VuoList_VuoUrl_isLessThan(a.urls, b.urls)) return true;
//	if (VuoList_VuoUrl_isLessThan(b.urls, a.urls)) return false;

	return false;
}
