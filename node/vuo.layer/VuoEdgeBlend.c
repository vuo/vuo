/**
 * @file
 * VuoEdgeBlend implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "type.h"
#include "VuoEdgeBlend.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Edge Blend",
					 "description" : "Apply a fade and gamma curve to an edge.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoReal"
					 ]
				 });
#endif
/// @}


/**
 * @ingroup VuoEdgeBlend
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     	"cutoff" : 0.5,
 *     	"gamma" : 1,
 *		"crop" : 0
 *   }
 * }
 */
VuoEdgeBlend VuoEdgeBlend_makeFromJson(json_object * js)
{
	VuoEdgeBlend edgeBlend = VuoEdgeBlend_make(0., 1., 0);

	json_object *o = NULL;

	if (json_object_object_get_ex(js, "cutoff", &o))
		edgeBlend.cutoff = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "gamma", &o))
		edgeBlend.gamma = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "crop", &o))
		edgeBlend.crop = VuoReal_makeFromJson(o);

	return edgeBlend;
}

/**
 * @ingroup VuoEdgeBlend
 * Encodes @c value as a JSON object.
 */
json_object * VuoEdgeBlend_getJson(const VuoEdgeBlend value)
{
	json_object *js = json_object_new_object();

	json_object *cutoffObject = VuoReal_getJson(value.cutoff);
	json_object_object_add(js, "cutoff", cutoffObject);

	json_object *gammaObject = VuoReal_getJson(value.gamma);
	json_object_object_add(js, "gamma", gammaObject);

	json_object *cropObject = VuoReal_getJson(value.crop);
	json_object_object_add(js, "crop", cropObject);

	return js;
}


/**
 * @ingroup VuoEdgeBlend
 * Returns a compact string representation of @c value.
 */
char * VuoEdgeBlend_getSummary(const VuoEdgeBlend value)
{
	return VuoText_format("Cutoff: %g, Gamma: %g  Crop: %g", value.cutoff, value.gamma, value.crop);
}
