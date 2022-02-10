/**
 * @file
 * vuo.ui.make.theme node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoUiTheme.h"
#include "VuoList_VuoUiTheme.h"

VuoModuleMetadata({
					  "title" : "Combine Widget Themes",
					  "keywords" : [
						  "interface", "gui", "user interface", "interact",
						  "group",
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "DisplayControlPanel.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoList_VuoUiTheme) elements,
	VuoOutputData(VuoUiTheme) theme
)
{
	*theme = VuoUiTheme_makeGroup(elements);
}
