/**
 * @file
 * vuo.text.make.controlCode node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <string.h>
#include "VuoControlCode.h"

VuoModuleMetadata({
					 "title" : "Make Text from Control Code",
					 "keywords" : [ "carriage", "return", "newline", "line", "\n", "\r", "tab", "em", "en", "space", "break", "invisible" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "SeparateWords.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoControlCode, {"default":"newline-unix"}) controlCode,
	VuoOutputData(VuoText) text
)
{
	*text = VuoControlCode_makeText( controlCode );
}
