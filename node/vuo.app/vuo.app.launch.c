/**
 * @file
 * vuo.app.launch node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoAppLaunch.h"

VuoModuleMetadata({
					 "title" : "Launch App",
					 "keywords" : [ "execute", "run", "start", "open", "application" ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoAppLaunch"
					 ],
					 "node": {
						 "isInterface" : true,
						 "exampleCompositions" : [ "LaunchCalculator.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"name":"URL", "default":"/Applications/Calculator.app"}) url,
		VuoInputEvent({"eventBlocking":"none","data":"url"}) urlEvent,
		VuoInputData(VuoBoolean, {"default":true}) activate
)
{
	if (urlEvent)
		VuoAppLaunch_launch(url, activate);
}
