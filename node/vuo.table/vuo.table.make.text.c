/**
 * @file
 * vuo.table.make.text node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoTable.h"

VuoModuleMetadata({
					  "title" : "Make Table from Text",
					  "keywords" : [
						"row", "column", "grid", "spreadsheet", "structure",
						"csv", "tsv", "comma", "tab", "separated",
						"parse", "convert", "read"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "ChangeTableDataToMetric.vuo", "GraphColdestTemperatures.vuo", "PerformCoinFlipExperiment.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoInputData(VuoTableFormat, {"default":"csv"}) format,
		VuoOutputData(VuoTable) table
)
{
	*table = VuoTable_makeFromText(text, format);
}
