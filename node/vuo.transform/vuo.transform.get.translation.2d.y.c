/**
 * @file
 * vuo.transform.get.translation.2d.y node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
	"title" : "Get 2D Transform Y Translation",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node": {
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoTransform2d) transform,
	VuoOutputData(VuoReal) yTranslation
)
{
	*yTranslation = transform.translation.y;
}
