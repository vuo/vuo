/**
 * @file
 * vuo.image.findBarcode node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoBarcode.h"

VuoModuleMetadata({
					  "title" : "Find Barcode in Image",
					  "keywords" : [
						  "search", "scanner",
						  "UPC",
						  "Code", "39", "128", "QR",
						  "DataMatrix"
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoBarcode"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ScanBarcodes.vuo", "FindBarcodeInSyphonImage.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputEvent({"eventBlocking":"door","data":"image"}) imageEvent,
		VuoOutputData(VuoText) barcode,
		VuoOutputEvent({"data":"barcode"}) barcodeEvent,
		VuoOutputData(VuoPoint2d) center,
		VuoOutputEvent({"data":"center"}) centerEvent,
		VuoOutputData(VuoReal) width,
		VuoOutputEvent({"data":"width"}) widthEvent,
		VuoOutputData(VuoReal) height,
		VuoOutputEvent({"data":"height"}) heightEvent
)
{
	VuoRectangle r;
	VuoText b = VuoBarcode_read(image, &r);
	if (b)
	{
		*barcodeEvent = true;
		*barcode = b;
		*centerEvent = true;
		*center = r.center;
		*widthEvent = true;
		*width = r.size.x;
		*heightEvent = true;
		*height = r.size.y;
	}
}
