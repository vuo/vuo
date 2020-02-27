/**
 * @file
 * VuoAnchor implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"

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
						"VuoText",
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
 *   "center-top"
 * }
 * @eg{
 *   {
 *     "verticalAlignment": "top",
 *     "horizontalAlignment": "center",
 *   }
 * }
 */
VuoAnchor VuoAnchor_makeFromJson(json_object *js)
{
	if (json_object_is_type(js, json_type_string))
	{
		char h[7];
		char v[7];
		sscanf(json_object_get_string(js), "%6[a-z]-%6[a-z]", h, v);
		return VuoAnchor_make(VuoHorizontalAlignment_makeFromJson(json_object_new_string(h)),
							  VuoVerticalAlignment_makeFromJson(json_object_new_string(v)));
	}

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
	json_object *horizontal = VuoHorizontalAlignment_getJson(VuoAnchor_getHorizontal(value));
	json_object *vertical = VuoVerticalAlignment_getJson(VuoAnchor_getVertical(value));
	char *combined = VuoText_format("%s-%s", json_object_get_string(horizontal), json_object_get_string(vertical));
	json_object_put(horizontal);
	json_object_put(vertical);
	json_object *js = json_object_new_string(combined);
	free(combined);
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

/**
 * Returns an offset (-.5, 0, or .5) for each axis of the anchor.
 * @version200New
 */
VuoPoint2d VuoAnchor_getOffset(VuoAnchor anchor)
{
	VuoHorizontalAlignment h = VuoAnchor_getHorizontal(anchor);
	VuoVerticalAlignment   v = VuoAnchor_getVertical(anchor);
	return (VuoPoint2d){
		h == VuoHorizontalAlignment_Left ?  .5 : (h == VuoHorizontalAlignment_Center ? 0 : -.5),
		v == VuoVerticalAlignment_Top    ? -.5 : (v == VuoVerticalAlignment_Center   ? 0 :  .5)
	};
}

/**
 * Returns true if the two values are equal.
 */
bool VuoAnchor_areEqual(const VuoAnchor value1, const VuoAnchor value2)
{
	return VuoAnchor_getHorizontal(value1) == VuoAnchor_getHorizontal(value2)
		&& VuoAnchor_getVertical(value1)   == VuoAnchor_getVertical(value2);
}

/**
 * Returns true if the value1 is less than value2.
 */
bool VuoAnchor_isLessThan(const VuoAnchor value1, const VuoAnchor value2)
{
	return VuoHorizontalAlignment_isLessThan(VuoAnchor_getHorizontal(value1), VuoAnchor_getHorizontal(value2))
		&& VuoVerticalAlignment_isLessThan(VuoAnchor_getVertical(value1), VuoAnchor_getVertical(value2));
}
