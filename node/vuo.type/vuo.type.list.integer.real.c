/**
 * @file
 * vuo.type.list.integer.real node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Integer List to Real List",
					  "description": "Outputs a list containing real number equivalents to the input list's integers.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoInteger) integers,
	VuoOutputData(VuoList_VuoReal) reals
)
{
	unsigned long count = VuoListGetCount_VuoInteger(integers);
	VuoInteger *inputs = VuoListGetData_VuoInteger(integers);
	*reals = VuoListCreateWithCount_VuoReal(count, 0);
	VuoReal *outputs = VuoListGetData_VuoReal(*reals);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = inputs[i];
}
