/**
 * @file
 * vuo.type.list.real.integer node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert Real List to Integer List",
					  "description": "Outputs a list containing integer approximations of the input list's real numbers.  Real numbers are rounded to the nearest integer.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoReal) reals,
	VuoOutputData(VuoList_VuoInteger) integers
)
{
	*integers = VuoListCreate_VuoInteger();
	unsigned long count = VuoListGetCount_VuoReal(reals);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoInteger(*integers, lround(VuoListGetValue_VuoReal(reals, i)));
}
