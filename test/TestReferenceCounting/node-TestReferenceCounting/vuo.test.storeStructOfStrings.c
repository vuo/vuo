/**
 * @file
 * vuo.test.storeStructOfStrings node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Store Struct of Strings",
					 "description" : "",
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoText"
					 ]
				 });

struct StructOfStrings
{
	VuoText current;
	VuoText previous;
};

struct StructOfStrings * nodeInstanceInit()
{
	struct StructOfStrings *instance = (struct StructOfStrings *)malloc(sizeof(struct StructOfStrings *));
	VuoRegister(instance, free);
	instance->current = NULL;
	instance->previous = NULL;
	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct StructOfStrings *) instance,
		VuoOutputData(VuoInteger) outEvent
)
{
	VuoText newCurrent = VuoText_make("initializer string");
	VuoRetain(newCurrent);
	VuoRelease((*instance)->previous);
	(*instance)->previous = (*instance)->current;
	(*instance)->current = newCurrent;
	*outEvent = 0;
}

void nodeInstanceFini
(
		VuoInstanceData(struct StructOfStrings *) instance
)
{
	VuoRelease((*instance)->current);
	VuoRelease((*instance)->previous);
}
