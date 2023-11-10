/**
 * @file
 * vuo.test.log node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Log",
					 "description" : "",
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoInputData(VuoText) logMessage,
		VuoInputEvent({"eventBlocking":"none","data":"message"}) logMessageEvent
)
{
	VUserLog("%s", logMessage);
}