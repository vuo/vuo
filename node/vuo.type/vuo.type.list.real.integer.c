/**
 * @file
 * vuo.type.list.real.integer node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to Integer List",
					  "description": "Outputs a list containing integer approximations of the input list's real numbers.  Real numbers are rounded to the nearest integer.",
					  "version": "1.0.1"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) reals,
	VuoOutputData(VuoList_VuoInteger) integers
)
{
	unsigned long count = VuoListGetCount_VuoReal(reals);
	VuoReal *inputs = VuoListGetData_VuoReal(reals);
	*integers = VuoListCreateWithCount_VuoInteger(count, 0);
	VuoInteger *outputs = VuoListGetData_VuoInteger(*integers);
	for (unsigned long i = 0; i < count; ++i)
		outputs[i] = lround(inputs[i]);
}
