/**
 * @file
 * VuoAnchor implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoAnchor.h"
#include "VuoList_VuoAnchor.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "Anchor",
					  "description" : "Horizontal + Vertical alignment.",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "dependencies" : [
						"VuoList_VuoAnchor",
						"VuoHorizontalAlignment",
						"VuoVerticalAlignment"
					  ]
				  });
#endif
/// @}

/**
 * @ingroup VuoAnchor
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "replaceThis" : -1
 *   }
 * }
 */
VuoAnchor VuoAnchor_makeFromJson(json_object * js)
{
	VuoAnchor anchor = { VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center };
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "horizontalAlignment", &o))
		anchor.horizontalAlignment = VuoHorizontalAlignment_makeFromJson(o);

	if (json_object_object_get_ex(js, "verticalAlignment", &o))
		anchor.verticalAlignment = VuoVerticalAlignment_makeFromJson(o);

	return anchor;
}

/**
 * @ingroup VuoAnchor
 * Encodes @c value as a JSON object.
 */
json_object * VuoAnchor_getJson(const VuoAnchor value)
{
	json_object *js = json_object_new_object();

	json_object *horizontal = VuoHorizontalAlignment_getJson(value.horizontalAlignment);
	json_object_object_add(js, "horizontalAlignment", horizontal);

	json_object *vertical = VuoVerticalAlignment_getJson(value.verticalAlignment);
	json_object_object_add(js, "verticalAlignment", vertical);

	return js;
}

/**
 * @ingroup VuoAnchor
 * Returns a compact string representation of @c value.
 */
char * VuoAnchor_getSummary(const VuoAnchor value)
{
	if(VuoAnchor_areEqual(value, VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center)))
		return strdup("Center");

	char* h = VuoHorizontalAlignment_getSummary(value.horizontalAlignment);
	char* v = VuoVerticalAlignment_getSummary(value.verticalAlignment);

	char* sum = (char*) malloc( sizeof(char) * (strlen(h) + strlen(v) + 2) );
	sprintf(sum, "%s %s", v, h);

	free(h);
	free(v);

	return sum;
}
