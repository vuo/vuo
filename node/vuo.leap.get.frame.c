/**
 * @file
 * vuo.leap.get.frame node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

const char *moduleName = "Get Frame Values";
const char *moduleDescription = "Gives the values in a Leap Frame.";
const char *moduleKeywords[] = { };
const unsigned int moduleVersion = 100;
const bool nodeIsInterface = false;

VuoModuleMetadata({
					  "title" : "Get Frame Values",
					  "description" :
						  "<p>Gives information about a frame from a Leap Motion device, \
						  including hands and pointables (fingers or tools).</p> \
						  <p>To see information for a specific hand or pointable, connect an output of this node to a \
						  `Get Item from List` (`vuo.list.get.VuoLeapHand` or `vuo.list.get.VuoLeapPointable`) node, \
						  and connect that to a `Get Hand Values` or `Get Pointable Values` node.</p> \
						  <p>To sort the hands or pointables, connect an output of this node to one of the \
						  `Sort Hands by ...` or `Sort Pointables by ...` nodes.</p> \
						  <p>To see only the hands or pointables that match certain criteria, connect an output of this node to \
						  one of the `Filter Hands by ...` or `Filter Pointables by ...` nodes.</p> \
						  <ul> \
						  <li>`id` — A unique ID for the frame. Each frame's ID number is one more than the previous frame's.</li> \
						  <li>`hands`— The hands detected in this frame, listed in arbitrary order.</li> \
						  <li>`pointables` — The fingers and tools detected in this frame, listed in arbitrary order.</li> \
						  </ul> \
						  ",
					  "keywords" : [ "gesture", "controller", "motion", "hand", "palm", "pointable", "finger", "tool" ],
					  "version" : "1.0.0",
					  "node": {
						  "isInterface" : false
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoLeapFrame) frame,
		VuoOutputData(VuoInteger) id,
		VuoOutputData(VuoList_VuoLeapHand) hands,
		VuoOutputData(VuoList_VuoLeapPointable) pointables
)
{
	*id = frame.id;
	*hands = frame.hands;
	*pointables = frame.pointables;
}
