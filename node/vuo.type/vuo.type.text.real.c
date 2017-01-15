/**
 * @file
 * vuo.type.text.real node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Convert Text to Real",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoText,{"default":""}) text,
		VuoOutputData(VuoReal) real
)
{
	if (!text)
		return;

	*real = VuoReal_makeFromString(text);
}
