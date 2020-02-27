/**
 * @file
 * VuoBarcode implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoBarcode.h"

#include <OpenGL/CGLMacro.h>

#include <zxing/common/GreyscaleLuminanceSource.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/ReaderException.h>

#include "module.h"

extern "C"
{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoBarcode",
					 "dependencies" : [
						 "VuoImage",
						 "VuoRectangle",
						 "VuoText",
						 "VuoList_VuoText",
						 "iconv",
						 "zxing"
					 ]
				 });
#endif
}

using namespace zxing;

/**
 * Attempts to find a barcode in @c image.
 * If a barcode was found, returns the decoded text,
 * and, if `outputPosition` is non-NULL, outputs the Vuo Coordinates covering the detected area.
 * If no barcode was found, returns NULL.
 */
VuoText VuoBarcode_read(VuoImage image, VuoInteger format, VuoText *outputFormat, VuoRectangle *outputPosition)
{
	if (!image)
		return NULL;

	// For full-color images, GL_LUMINANCE only returns the red channel,
	// but that's typically good enough for barcode detection
	// (and much faster than doing a CPU-side grayscale mix).
	const unsigned char *luma = VuoImage_getBuffer(image, GL_LUMINANCE);
	unsigned char *lumaFlipped = (unsigned char *)malloc(image->pixelsWide * image->pixelsHigh);
	for (int y = 0; y < image->pixelsHigh; ++y)
		memcpy(lumaFlipped + (image->pixelsHigh - y - 1) * image->pixelsWide, luma + y * image->pixelsWide, image->pixelsWide);

	try
	{
		ArrayRef<char> d((char *)lumaFlipped, image->pixelsWide * image->pixelsHigh);
		Ref<GreyscaleLuminanceSource> source(new GreyscaleLuminanceSource(d, image->pixelsWide, image->pixelsHigh, 0, 0, image->pixelsWide, image->pixelsHigh));
//		Ref<Binarizer> binarizer(new HybridBinarizer(source));
		Ref<Binarizer> binarizer(new GlobalHistogramBinarizer(source));
		Ref<BinaryBitmap> bir(new BinaryBitmap(binarizer));

		DecodeHints hints(DecodeHints::DEFAULT_HINT);
//		hints.setTryHarder(true);

		if (format != 0)
		{
			hints.clear();
			if      (format ==  10) hints.addFormat(BarcodeFormat::CODABAR);
			else if (format ==  20) hints.addFormat(BarcodeFormat::CODE_39);
			else if (format ==  30) hints.addFormat(BarcodeFormat::CODE_93);
			else if (format ==  40) hints.addFormat(BarcodeFormat::CODE_128);
			else if (format ==  50) hints.addFormat(BarcodeFormat::EAN_8);
			else if (format ==  60) hints.addFormat(BarcodeFormat::EAN_13);
			else if (format ==  70) hints.addFormat(BarcodeFormat::RSS_14);
			else if (format ==  80) hints.addFormat(BarcodeFormat::RSS_EXPANDED);
			else if (format ==  90) hints.addFormat(BarcodeFormat::ITF);
			else if (format == 100) hints.addFormat(BarcodeFormat::UPC_A);
			else if (format == 110) hints.addFormat(BarcodeFormat::UPC_E);
			else if (format == 200) hints.addFormat(BarcodeFormat::AZTEC);
			else if (format == 210) hints.addFormat(BarcodeFormat::DATA_MATRIX);
			else if (format == 220) hints.addFormat(BarcodeFormat::MAXICODE);
			else if (format == 230) hints.addFormat(BarcodeFormat::PDF_417);
			else if (format == 240) hints.addFormat(BarcodeFormat::QR_CODE);
		}

		MultiFormatReader reader;
		/// @todo Check whether MultiFormatReader::decodeWithState is faster
		Ref<Result> result = reader.decode(bir, hints);
		Ref<String> s = result->getText();
		if (outputPosition)
		{
			// ResultPoints may be 4 corners of the barcode, or 2 line segment endpoints.
			// Turn them into a typical axis-aligned Vuo Coordinates center/size pair.
			ArrayRef< Ref<ResultPoint> > points = result->getResultPoints();
			bool first = true;
			for (int i = 0; i < points->size(); ++i)
			{
				VuoRectangle r = VuoRectangle_make( points[i]->getX()/(image->pixelsWide/2) - 1,
												   (image->pixelsHigh/2 - points[i]->getY())/(image->pixelsWide/2),
												   0, 0);
				if (first)
				{
					first = false;
					*outputPosition = r;
				}
				else
					*outputPosition = VuoRectangle_union(*outputPosition, r);
			}
		}
		free(lumaFlipped);

		int actualFormat = result->getBarcodeFormat().value;
		if      (actualFormat == BarcodeFormat::CODABAR)      *outputFormat = VuoText_make("Codabar");
		else if (actualFormat == BarcodeFormat::CODE_39)      *outputFormat = VuoText_make("Code 39");
		else if (actualFormat == BarcodeFormat::CODE_93)      *outputFormat = VuoText_make("Code 93");
		else if (actualFormat == BarcodeFormat::CODE_128)     *outputFormat = VuoText_make("Code 128");
		else if (actualFormat == BarcodeFormat::EAN_8)        *outputFormat = VuoText_make("EAN-8");
		else if (actualFormat == BarcodeFormat::EAN_13)       *outputFormat = VuoText_make("EAN-13");
		else if (actualFormat == BarcodeFormat::RSS_14)       *outputFormat = VuoText_make("GS1 DataBar-14 (RSS-14)");
		else if (actualFormat == BarcodeFormat::RSS_EXPANDED) *outputFormat = VuoText_make("GS1 DataBar Expanded (RSS Expanded)");
		else if (actualFormat == BarcodeFormat::ITF)          *outputFormat = VuoText_make("Interleaved 2 of 5 (ITF)");
		else if (actualFormat == BarcodeFormat::UPC_A)        *outputFormat = VuoText_make("UPC-A");
		else if (actualFormat == BarcodeFormat::UPC_E)        *outputFormat = VuoText_make("UPC-E");
		else if (actualFormat == BarcodeFormat::AZTEC)        *outputFormat = VuoText_make("Aztec");
		else if (actualFormat == BarcodeFormat::DATA_MATRIX)  *outputFormat = VuoText_make("Data Matrix");
		else if (actualFormat == BarcodeFormat::MAXICODE)     *outputFormat = VuoText_make("MaxiCode");
		else if (actualFormat == BarcodeFormat::PDF_417)      *outputFormat = VuoText_make("PDF417");
		else if (actualFormat == BarcodeFormat::QR_CODE)      *outputFormat = VuoText_make("QR Code");
		else                                                  *outputFormat = VuoText_make("(unknown)");

		unsigned long len = s->length();
		char *outputText = (char *)malloc(len+1);
		for(int i = 0; i < len; ++i)
			outputText[i] = s->charAt(i);
		outputText[len] = 0;
		VuoRegister(outputText, free);
		return outputText;
	}
	catch (Exception e)
	{
		// Why does zxing throw an exception for the perfectly normal no-code-detected situation.
		if (strcmp(e.what(), "No code detected"))
			VDebugLog("Exception: %s", e.what());
	}

	free(lumaFlipped);
	return NULL;
}
