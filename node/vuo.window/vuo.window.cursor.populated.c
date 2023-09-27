/**
 * @file
 * vuo.window.cursor.populated node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Is Cursor Populated",
					 "keywords" : [ "pointer", "hand", "arrow", "hide", "touch", "properties", "none", "empty", "non-empty", "nonempty" ],
					 "version" : "1.0.0",
					 "node": {
						"isDeprecated": true,
						"exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoCursor, {"default":"none"}) cursor,
		VuoOutputData(VuoBoolean) populated
)
{
		*populated = VuoCursor_isPopulated(cursor);
}
