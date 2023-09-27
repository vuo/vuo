/**
 * @file
 * example.stateful.countExecutions node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

VuoModuleMetadata({
					 "title" : "Count Executions",
					 "description" : "Keeps track of how many times this node has been executed, and outputs that count.",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [ ],
				 });

VuoInteger * nodeInstanceInit()
{
	VuoInteger *countState = (VuoInteger *) malloc(sizeof(VuoInteger));
	VuoRegister(countState, free);
	*countState = 0;
	return countState;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoInteger *) countState,
		VuoInputEvent() event,
		VuoOutputData(VuoInteger) count
)
{
	*count = ++(**countState);
}

void nodeInstanceFini(VuoInstanceData(VuoInteger *) countState)
{
}
