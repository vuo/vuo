/**
 * @file
 * vuo.math.isLessThan.integer node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Is Less Than",
					 "keywords" : [ "least", "small", "few", "low", "<", "compare", "conditional" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) a,
		VuoInputData(VuoInteger, {"default":0}) b,
		VuoOutputData(VuoBoolean) lessThan
)
{
	*lessThan = a < b;
}
