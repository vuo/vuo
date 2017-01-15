/**
 * @file
 * vuo.type.text.data node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoData.h"

VuoModuleMetadata({
					  "title" : "Convert Text to Data",
					  "keywords" : [ ],
					  "version" : "1.0.0",
				  });

void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoOutputData(VuoData) data
)
{
	if (!text)
		return;

	*data = VuoData_makeFromText(text);
}

