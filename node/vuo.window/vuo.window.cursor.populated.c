/**
 * @file
 * vuo.window.cursor.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Is Cursor Populated",
					 "keywords" : [ "pointer", "hand", "arrow", "hide", "touch", "properties", "none", "non-empty", "nonempty" ],
					 "version" : "1.0.0",
					 "node": {
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
