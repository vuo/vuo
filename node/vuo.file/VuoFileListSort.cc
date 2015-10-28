/**
 * @file
 * VuoFileListSort implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoFileListSort.h"
#include <vector>
#include <algorithm>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoFileListSort",
					  "dependencies" : [
						"VuoText",
						"VuoList_VuoText"
					  ]
				 });
#endif
}

/**
 * Sorts the texts into case-sensitive lexicographic order.
 */
void VuoListSort_VuoText(VuoList_VuoText list)
{
	std::vector<VuoText> *l = (std::vector<VuoText> *)list;
	std::sort(l->begin(), l->end(), VuoText_isLessThan);
}
