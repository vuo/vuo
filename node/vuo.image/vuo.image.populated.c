/**
 * @file
 * vuo.image.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Is Image Populated",
					 "keywords" : [ "width", "height", "dimensions", "defined", "nonempty", "non-empty" ],
					 "version" : "1.0.0",
					 "node" : {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoImage) image,
	VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoImage_isPopulated(image);
}
