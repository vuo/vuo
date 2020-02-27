/**
 * @file
 * vuo.tree.format.xml node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoTree.h"

VuoModuleMetadata({
					  "title" : "Format Tree as XML",
					  "keywords" : [
						"text", "convert", "serialize", "export", "save", "write"
					  ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "RecordMouseClicksToJsonFile.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoTree) tree,
		VuoInputData(VuoBoolean, {"default":true}) indent,
		VuoOutputData(VuoText, {"name":"XML"}) xml
)
{
	*xml = VuoTree_serializeAsXml(tree, indent);
}
