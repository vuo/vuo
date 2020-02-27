/**
 * @file
 * VuoLeapFrame implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "type.h"
#include "VuoLeapFrame.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					"title" : "Leap Frame",
					"description" : "Frame of Leap Motion data.",
					"keywords" : ["gesture", "controller", "motion", "hand", "finger"],
					"version" : "1.0.0",
					"dependencies" : [
					   "VuoInteger",
					   "VuoLeapHand",
					   "VuoLeapPointable",
					   "VuoText",
					   "VuoList_VuoLeapHand",
					   "VuoList_VuoLeapPointable"
					]
					});
#endif
/// @}

/**
 * @ingroup VuoLeapFrame
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "id" : 42
 *   }
 * }
 */
VuoLeapFrame VuoLeapFrame_makeFromJson(json_object * js)
{
	return (VuoLeapFrame){
		VuoJson_getObjectValue(VuoInteger,               js, "id",         -1),
		VuoJson_getObjectValue(VuoList_VuoLeapHand,      js, "hands",      NULL),
		VuoJson_getObjectValue(VuoList_VuoLeapPointable, js, "pointables", NULL)
	};
}

/**
 * @ingroup VuoLeapFrame
 * Encodes @c value as a JSON object.
 */
json_object * VuoLeapFrame_getJson(const VuoLeapFrame value)
{
	json_object *js = json_object_new_object();
	json_object_object_add(js, "id",         VuoInteger_getJson(value.id));
	json_object_object_add(js, "hands",      VuoList_VuoLeapHand_getJson(value.hands));
	json_object_object_add(js, "pointables", VuoList_VuoLeapPointable_getJson(value.pointables));
	return js;
}

/**
 * @ingroup VuoLeapFrame
 * Returns a compact string representation of @c value.
 */
char * VuoLeapFrame_getSummary(const VuoLeapFrame value)
{
	return VuoText_format("%lld", value.id);
}

/**
 * @ingroup VuoLeapFrame
 * Creates a new frame from the specified values.
 */
VuoLeapFrame VuoLeapFrame_make(VuoInteger id, VuoList_VuoLeapHand hands, VuoList_VuoLeapPointable pointables)
{
	VuoLeapFrame frame;
	frame.id = id;
	frame.pointables = pointables;
	frame.hands = hands;
	return frame;
}
