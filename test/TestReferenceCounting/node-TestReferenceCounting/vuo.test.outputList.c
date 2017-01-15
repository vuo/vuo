/**
 * @file
 * vuo.test.outputList node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Output List",
					 "description" : "",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoList_VuoText) listOut,
		VuoOutputData(VuoInteger) outEvent
)
{
	VuoList_VuoText l = VuoListCreate_VuoText();
	VuoText firstElement = VuoText_make("first");
	VuoListAppendValue_VuoText(l, firstElement);
	VuoText secondElement = VuoText_make("second");
	VuoListAppendValue_VuoText(l, secondElement);
	*listOut = l;
	*outEvent = 0;
}
