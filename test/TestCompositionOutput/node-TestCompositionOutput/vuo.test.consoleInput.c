/**
 * @file
 * vuo.test.consoleInput node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <dispatch/dispatch.h>
#include <stdio.h>

VuoModuleMetadata({
					 "title" : "Simulate Console Input",
					 "description" : "Simulates console input of the first words of the 'lorem ipsum' text.",
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : true
					 }
				 });

void nodeEvent
(
	VuoOutputTrigger(typedWord,VuoText)
)
{
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
							   const size_t wordCount = 5;
							   const char * words[] = { "Lorem", "ipsum", "dolor", "sit", "amet" };
							   for (int i = 0; i < wordCount; ++i)
							   {
								   VuoText s = VuoText_make(words[i]);
								   typedWord(s);
							   }
						   });
}
