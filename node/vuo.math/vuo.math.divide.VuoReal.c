/**
 * @file
 * vuo.math.divide.VuoReal node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Divide",
					 "keywords" : [ "quotient", "fraction", "/", "÷", "arithmetic", "calculate" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoReal, {"default":0.0}) a,
	VuoInputData(VuoReal, {"default":1.0}) b,
	VuoOutputData(VuoReal) quotient
)
{
	*quotient = a / b;
}
