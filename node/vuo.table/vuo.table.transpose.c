/**
 * @file
 * vuo.table.transpose node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Transpose Table",
					  "keywords" : [
						  "diagonal", "swap", "interchange",
						  "row", "column", "cell", "item"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTable) table,
		VuoOutputData(VuoTable) flippedTable
)
{
	*flippedTable = VuoTable_transpose(table);
}
