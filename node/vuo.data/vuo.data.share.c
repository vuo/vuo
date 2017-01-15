/**
 * @file
 * vuo.data.share node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Share Value",
					  "keywords" : [ "splitter", "input splitter", "output splitter", "hold", "pass", "preserve", "keep",
						"constant", "identity", "convert" ],
					  "version" : "1.0.0"
				  });

void nodeEvent
(
		VuoInputData(VuoGenericType1) value,
		VuoOutputData(VuoGenericType1) sameValue
)
{
	*sameValue = value;
}
