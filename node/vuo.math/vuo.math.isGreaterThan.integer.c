/**
 * @file
 * vuo.math.isGreaterThan.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is Greater Than",
					 "keywords" : [ "large", "big", "high", "more", "most", ">", "compare", "conditional" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) a,
		VuoInputData(VuoInteger, {"default":0}) b,
		VuoOutputData(VuoBoolean) greaterThan
 )
{
	*greaterThan = a > b;
}
