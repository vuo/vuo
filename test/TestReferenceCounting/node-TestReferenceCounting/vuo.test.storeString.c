/**
 * @file
 * vuo.test.storeString node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Store String",
					 "description" : "",
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoText"
					 ]
				 });

VuoText nodeInstanceInit()
{
	return NULL;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoText) instance,
		VuoOutputData(VuoInteger) outEvent
)
{
	VuoText newInstance = VuoText_make("initializer string");
	*instance = newInstance;
	*outEvent = 0;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoText) instance
)
{
}
