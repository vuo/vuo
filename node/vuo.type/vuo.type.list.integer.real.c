/**
 * @file
 * vuo.type.list.integer.real node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Integer List to Real List",
					  "description": "Outputs a list containing real number equivalents to the input list's integers.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoInteger) integers,
	VuoOutputData(VuoList_VuoReal) reals
)
{
	*reals = VuoListCreate_VuoReal();
	unsigned long count = VuoListGetCount_VuoInteger(integers);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoReal(*reals, VuoListGetValue_VuoInteger(integers, i));
}
