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
	json_object *o = NULL;

	VuoHorizontalAlignment horizontal = VuoHorizontalAlignment_Center;
	if (json_object_object_get_ex(js, "horizontalAlignment", &o))
		horizontal = VuoHorizontalAlignment_makeFromJson(o);

	VuoVerticalAlignment vertical = VuoVerticalAlignment_Center;
	if (json_object_object_get_ex(js, "verticalAlignment", &o))
		vertical = VuoVerticalAlignment_makeFromJson(o);

	return VuoAnchor_make(horizontal, vertical);
}

/**
 * @ingroup VuoAnchor
 * Encodes @c value as a JSON object.
 */
json_object * VuoAnchor_getJson(const VuoAnchor value)
{
	json_object *js = json_object_new_object();

	json_object *horizontal = VuoHorizontalAlignment_getJson(VuoAnchor_getHorizontal(value));
	json_object_object_add(js, "horizontalAlignment", horizontal);

	json_object *vertical = VuoVerticalAlignment_getJson(VuoAnchor_getVertical(value));
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

	char *h = VuoHorizontalAlignment_getSummary(VuoAnchor_getHorizontal(value));
	char *v = VuoVerticalAlignment_getSummary(VuoAnchor_getVertical(value));

	char* sum = (char*) malloc( sizeof(char) * (strlen(h) + strlen(v) + 2) );
	sprintf(sum, "%s %s", v, h);

	free(h);
	free(v);

	return sum;
}

/**
 * Returns a list of values that instances of this type can have.
 */
VuoList_VuoAnchor VuoAnchor_getAllowedValues(void)
{
	VuoList_VuoAnchor l = VuoListCreate_VuoAnchor();
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Left,   VuoVerticalAlignment_Top   ));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Top   ));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Right,  VuoVerticalAlignment_Top   ));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Left,   VuoVerticalAlignment_Center));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Right,  VuoVerticalAlignment_Center));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Left,   VuoVerticalAlignment_Bottom));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Bottom));
	VuoListAppendValue_VuoAnchor(l, VuoAnchor_make(VuoHorizontalAlignment_Right,  VuoVerticalAlignment_Bottom));
	return l;
}
