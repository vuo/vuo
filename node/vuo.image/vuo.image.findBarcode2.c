/**
 * @file
 * vuo.image.findBarcode node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
					  "version" : "2.0.0",
					  "dependencies" : [
						  "VuoBarcode"
					  ],
					  "node" : {
						  "exampleCompositions" : [ "ScanBarcodes.vuo", "FindBarcodeInImage.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputEvent({"eventBlocking":"door","data":"image"}) imageEvent,
		VuoInputData(VuoInteger, {"menuItems":[
			{"value":  0, "name":"Auto"},
			"---",
			"1D",
			{"value": 10, "name":"    Codabar"},
			{"value": 20, "name":"    Code 39"},
			{"value": 30, "name":"    Code 93"},
			{"value": 40, "name":"    Code 128"},
			{"value": 50, "name":"    EAN-8"},
			{"value": 60, "name":"    EAN-13"},
//			{"value": 70, "name":"    GS1 DataBar-14 (RSS-14)"},             // Not implemented in zxing-cpp.
//			{"value": 80, "name":"    GS1 DataBar Expanded (RSS Expanded)"}, // Not implemented in zxing-cpp.
			{"value": 90, "name":"    Interleaved 2 of 5 (ITF)"},
			{"value":100, "name":"    UPC-A"},
			{"value":110, "name":"    UPC-E"},
			"---",
			"2D",
			{"value":200, "name":"    Aztec"},
			{"value":210, "name":"    Data Matrix"},
//			{"value":220, "name":"    MaxiCode"},                            // Not implemented in zxing-cpp.
			{"value":230, "name":"    PDF417"},
			{"value":240, "name":"    QR Code"},
		], "default":0}) format,
		VuoInputEvent({"eventBlocking":"door","data":"format"}) formatEvent,

		VuoOutputData(VuoText) barcode,
		VuoOutputEvent({"data":"barcode"}) barcodeEvent,
		VuoOutputData(VuoText, {"name":"Format"}) foundFormat,
		VuoOutputEvent({"data":"foundFormat"}) foundFormatEvent,
		VuoOutputData(VuoRectangle) rectangle,
		VuoOutputEvent({"data":"rectangle"}) rectangleEvent
)
{
	VuoText b = VuoBarcode_read(image, format, foundFormat, rectangle);
	if (b)
	{
		*barcodeEvent = true;
		*barcode = b;
		*foundFormatEvent = true;
		*rectangleEvent = true;
	}
}
