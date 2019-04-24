/**
 * @file
 * vuo.image.findBarcode node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoBarcode.h"

VuoModuleMetadata({
					  "title" : "Find Barcode in Image",
					  "keywords" : [
						  "search", "scanner",
						  "UPC-A", "UPC-E",
						  "EAN-8", "EAN-13",
						  "Codabar",
						  "Code", "39", "93", "128", "QR",
						  "Aztec",
						  "DataMatrix", "Data Matrix",
						  "PDF417",
						  "Interleaved 2 of 5", "ITF",
					  ],
					  "version" : "1.1.0",
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
		VuoInputData(VuoInteger, {"menuItems":{
			 "0":"Auto",
			 "Separator0":"-",
			 "Section 1D":"1D",
			 "10":"    Codabar",
			 "20":"    Code 39",
			 "30":"    Code 93",
			 "40":"    Code 128",
			 "50":"    EAN-8",
			 "60":"    EAN-13",
//			 "70":"    GS1 DataBar-14 (RSS-14)",             // Not implemented in zxing-cpp.
//			 "80":"    GS1 DataBar Expanded (RSS Expanded)", // Not implemented in zxing-cpp.
			 "90":"    Interleaved 2 of 5 (ITF)",
			"100":"    UPC-A",
			"110":"    UPC-E",
			"Separator1":"-",
			"Section 2D":"2D",
			"200":"    Aztec",
			"210":"    Data Matrix",
//			"220":"    MaxiCode",                            // Not implemented in zxing-cpp.
			"230":"    PDF417",
			"240":"    QR Code"
		},"default":0}) format,
		VuoInputEvent({"eventBlocking":"door","data":"format"}) formatEvent,

		VuoOutputData(VuoText) barcode,
		VuoOutputEvent({"data":"barcode"}) barcodeEvent,
		VuoOutputData(VuoText, {"name":"Format"}) foundFormat,
		VuoOutputEvent({"data":"foundFormat"}) foundFormatEvent,
		VuoOutputData(VuoPoint2d) center,
		VuoOutputEvent({"data":"center"}) centerEvent,
		VuoOutputData(VuoReal) width,
		VuoOutputEvent({"data":"width"}) widthEvent,
		VuoOutputData(VuoReal) height,
		VuoOutputEvent({"data":"height"}) heightEvent
)
{
	VuoRectangle r;
	VuoText b = VuoBarcode_read(image, format, foundFormat, &r);
	if (b)
	{
		*barcodeEvent = true;
		*barcode = b;
		*foundFormatEvent = true;
		*centerEvent = true;
		*center = r.center;
		*widthEvent = true;
		*width = r.size.x;
		*heightEvent = true;
		*height = r.size.y;
	}
}
