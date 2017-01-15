/**
 * @file
 * vuo.list.shuffle node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Shuffle List",
					  "keywords" : [ "random", "pseudorandom", "PRNG", "RNG", "reorder", "sort", "permutation" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ShuffleLetters.vuo" ]
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) list,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":1.0}) chaos,
		VuoOutputData(VuoList_VuoGenericType1) shuffledList
)
{
	*shuffledList = VuoListCopy_VuoGenericType1(list);
	VuoListShuffle_VuoGenericType1(*shuffledList, chaos);
}
