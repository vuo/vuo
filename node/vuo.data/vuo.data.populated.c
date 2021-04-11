/**
 * @file
 * vuo.data.populated node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
	"title" : "Is Populated",
	"keywords" : [ "empty", "non-empty", "nonempty" ],
	"version" : "1.0.0",
	"genericTypes" : {
		"VuoGenericType1" : {
			"compatibleTypes" : [
				"VuoAudioSamples",
				"VuoCursor",
				"VuoImage",
				"VuoLayer",
				"VuoSceneObject",
				"VuoShader",
				"VuoText",
			]
		}
	},
});

void nodeEvent(
	VuoInputData(VuoGenericType1) value,
	VuoOutputData(VuoBoolean) populated)
{
	*populated = VuoGenericType1_isPopulated(value);
}
