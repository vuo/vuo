/**
 * @file
 * VuoBarcode implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoBarcode.h"

#include <OpenGL/CGLMacro.h>

#include <zxing/common/GreyscaleLuminanceSource.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/ReaderException.h>

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoBarcode",
					 "dependencies" : [
						 "VuoImage",
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
VuoText VuoBarcode_read(VuoImage image, VuoRectangle *outputPosition)
{
	if (!image)
		return NULL;

	// Download the full-color image from the GPU, and mix it down (CCIR601) to a single luminance byte
	// (can't use GL_LUMINANCE since it ignores the green and blue channels).
	const unsigned char *bgra = VuoImage_getBuffer(image, GL_BGRA);
	unsigned char *luma = (unsigned char *)malloc(image->pixelsWide*image->pixelsHigh);
	for (int y = 0; y < image->pixelsHigh; ++y)
		for (int x = 0; x < image->pixelsWide; ++x)
			luma[(image->pixelsHigh-y-1)*image->pixelsWide + x] =
					  0.299 * bgra[(y*image->pixelsWide + x)*4 + 2]
					+ 0.587 * bgra[(y*image->pixelsWide + x)*4 + 1]
					+ 0.114 * bgra[(y*image->pixelsWide + x)*4 + 0];

	try
	{
		ArrayRef<char> d((char *)luma, image->pixelsWide * image->pixelsHigh);
		Ref<GreyscaleLuminanceSource> source(new GreyscaleLuminanceSource(d, image->pixelsWide, image->pixelsHigh, 0, 0, image->pixelsWide, image->pixelsHigh));
//		Ref<Binarizer> binarizer(new HybridBinarizer(source));
		Ref<Binarizer> binarizer(new GlobalHistogramBinarizer(source));
		Ref<BinaryBitmap> bir(new BinaryBitmap(binarizer));

		DecodeHints hints(DecodeHints::DEFAULT_HINT);
//		hints.setTryHarder(true);

		MultiFormatReader reader;
		Ref<Result> result = reader.decode(bir, hints);
		Ref<String> s = result->getText();
		if (outputPosition)
		{
			// ResultPoints may be 4 corners of the barcode, or 2 line segment endpoints.
			// Turn them into a typical axis-aligned Vuo Coordinates center/size pair.
			ArrayRef< Ref<ResultPoint> > points = result->getResultPoints();
			*outputPosition = VuoRectangle_make(0,0,0,0);
			for (int i = 0; i < points->size(); ++i)
			{
				VuoRectangle r = VuoRectangle_make( points[i]->getX()/(image->pixelsWide/2) - 1,
												   -points[i]->getY()/(image->pixelsWide/2) + 1,
												   0, 0);
				*outputPosition = VuoPoint2d_rectangleUnion(*outputPosition, r);
			}
		}
//		VLog("found code type %s",BarcodeFormat::barcodeFormatNames[result->getBarcodeFormat().value]);
		free(luma);

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

	free(luma);
	return NULL;
}
