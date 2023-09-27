/**
 * @file
 * vuo.ui.get.interactions node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRenderedLayers.h"

VuoModuleMetadata(
{
	"title" : "Get Interactions",
	"keywords" : [ ],
	"version" : "1.0.0",
	"node" :
	{
		"exampleCompositions" : [ ]
	}
});

void nodeEvent
(
	VuoInputData(VuoRenderedLayers) renderedLayers,
	VuoOutputData(VuoList_VuoInteraction) interactions
)
{
	*interactions = renderedLayers.interactions;
}
