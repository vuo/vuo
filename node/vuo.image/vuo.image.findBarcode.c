/**
 * @file
 * vuo.image.findBarcode node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoBarcode.h"

VuoModuleMetadata({
					  "title" : "Find Barcode in Image",
					  "keywords" : [
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
		VuoInputEvent(VuoPortEventBlocking_Door, image) imageEvent,
		VuoOutputData(VuoText) barcode,
		VuoOutputEvent(barcode) barcodeEvent,
		VuoOutputData(VuoPoint2d) center,
		VuoOutputEvent(center) centerEvent,
		VuoOutputData(VuoReal) width,
		VuoOutputEvent(width) widthEvent,
		VuoOutputData(VuoReal) height,
		VuoOutputEvent(height) heightEvent
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
