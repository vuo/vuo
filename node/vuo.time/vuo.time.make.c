/**
 * @file
 * vuo.time.make node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTime.h"

VuoModuleMetadata({
					 "title" : "Make Date-Time",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions": [ "ShowElapsedTime.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger) year, // Default is set by VuoEditorComposition::setCustomConstantsForNewNode()
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":12}) month,
		VuoInputData(VuoInteger, {"default":1,"suggestedMin":1,"suggestedMax":31}) day,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":23}) hour,
		VuoInputData(VuoInteger, {"default":0,"suggestedMin":0,"suggestedMax":59}) minute,
		VuoInputData(VuoReal, {"default":0,"suggestedMin":0,"suggestedMax":59.99}) second,
		VuoOutputData(VuoTime) time
)
{
	*time = VuoTime_make(year, month, day, hour, minute, second);
}
