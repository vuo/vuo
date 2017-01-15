/**
 * @file
 * TestVuoRunnerCocoa implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#include <OpenGL/CGLMacro.h>

#define NS_RETURNS_INNER_POINTER
#include <QuartzCore/CoreImage.h>
#include <QuartzCore/CoreVideo.h>

#include <Vuo/Vuo.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#include <QtTest/QtTest>
#pragma clang diagnostic pop

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoImage);

typedef QMap<QString, QString> MapOfStrings;
typedef QPair<QString, MapOfStrings> PortAndDetails;

@implementation NSString(StringValue)
- (NSString *)stringValue
{
	return self;
}
@end

@implementation NSData(StringValue)
- (NSString *)stringValue
{
	size_t len = [self length];
	if (len == sizeof(double)*3)
	{
		double *d = (double *)[self bytes];
		return [NSString stringWithFormat:@"%g,%g,%g",d[0],d[1],d[2]];
	}
	return @"(unknown)";
}
@end

@implementation NSColor(StringValue)
- (NSString *)stringValue
{
	return [NSString stringWithFormat:@"%g,%g,%g,%g",[self redComponent],[self greenComponent],[self blueComponent],[self alphaComponent]];
}
@end

// Assumed to be a menuItems array.
@implementation NSArray(StringValue)
- (NSString *)stringValue
{
	NSMutableString *s = [NSMutableString new];
	[self enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop){
		[s appendFormat:@"%@,",[obj valueForKey:@"name"]];
	}];
	return s;
}
@end

void freeCallback(VuoImage imageToFree) {}

void cvPixelBufferFreeCallback(void *releaseRefCon, const void *baseAddress) {}

/**
 * Tests the @c VuoProtocol class.
 */
class TestVuoRunnerCocoa : public QObject
{
	Q_OBJECT

	NSImage *reddishImage;
	NSColor *blueishColor;
	NSColor *greenishColor;
	VuoImageFilter *sharedImageFilter;
	VuoImageFilter *sharedPassthruImageFilter;
	VuoImageGenerator *sharedImageGenerator;
	id activeRunner;

private slots:

	void initTestCase()
	{
		// Create a reddish image with a pinkish pixel in the bottom right quadrant.
		{
			unsigned char *inputPixels = (unsigned char *)malloc(10*10*4);
			for (int i = 0; i < 10*10; ++i)
			{
				inputPixels[i*4+0] = 255;
				inputPixels[i*4+1] = 127;
				inputPixels[i*4+2] = 127;
				inputPixels[i*4+3] = 255;
			}
			inputPixels[10*7*4 + 7*4 + 0] = 255;
			inputPixels[10*7*4 + 7*4 + 1] = 192;
			inputPixels[10*7*4 + 7*4 + 2] = 192;
			inputPixels[10*7*4 + 7*4 + 3] = 255;
			NSBitmapImageRep *inputBIR = [[NSBitmapImageRep alloc]
					initWithBitmapDataPlanes:&inputPixels
					pixelsWide:10
					pixelsHigh:10
					bitsPerSample:8
					samplesPerPixel:4
					hasAlpha:YES
					isPlanar:NO
					colorSpaceName:NSDeviceRGBColorSpace
					bytesPerRow:10*4
					bitsPerPixel:32];
			QVERIFY(inputBIR);
			reddishImage = [[NSImage alloc] initWithSize:NSMakeSize(10, 10)];
			QVERIFY(reddishImage);
			[reddishImage addRepresentation:inputBIR];
		}

		CGFloat c[4] = {.5, .5, 1, 1};
		blueishColor = [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:c count:4];

		CGFloat greenish[4] = {.5, 1, .5, 1};
		greenishColor = [NSColor colorWithColorSpace:[NSColorSpace sRGBColorSpace] components:greenish count:4];

		sharedImageFilter = [[VuoImageFilter alloc] initWithComposition:[NSURL fileURLWithPath:@"composition/ImageFilterRipple.vuo"]];
		QVERIFY(sharedImageFilter);

		sharedPassthruImageFilter = [[VuoImageFilter alloc] initWithComposition:[NSURL fileURLWithPath:@"composition/ImageFilterPassthru.vuo"]];
		QVERIFY(sharedPassthruImageFilter);

		sharedImageGenerator = [[VuoImageGenerator alloc] initWithComposition:[NSURL fileURLWithPath:@"composition/ImageGeneratorColor.vuo"]];
		QVERIFY(sharedImageGenerator);

		activeRunner = nil;
	}

	NSURL *url(void)
	{
		return [NSURL fileURLWithPath:[NSString stringWithUTF8String:(string("composition/") + QTest::currentDataTag()).c_str()]];
	}

	NSString *compositionString(void)
	{
		return [NSString stringWithContentsOfURL:url() encoding:NSUTF8StringEncoding error:NULL];
	}

	QList<QString> qListQStringWithNSArray(NSArray *arrayOfStrings)
	{
		__block QList<QString> list;
		[arrayOfStrings enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop){
			list << QString([obj UTF8String]);
		}];
		return list;
	}

	QMap<QString, QString> qMapQStringWithNSDictionary(NSDictionary *dictionaryOfStrings)
	{
		__block QMap<QString, QString> map;
		[dictionaryOfStrings enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop){
			map[[key UTF8String]] = [[obj stringValue] UTF8String];
		}];
		return map;
	}

	NSString *nsStringWithQString(QString s)
	{
		return [NSString stringWithUTF8String:s.toUtf8().data()];
	}

	void testCanOpenComposition_data()
	{
		QTest::addColumn<QString>("className");
		QTest::addColumn<bool>("expectedCompliance");

		QTest::newRow("ImageFilter.vuo")	<< "VuoImageFilter"		<< true;
		QTest::newRow("ImageFilter.vuo")	<< "VuoImageGenerator"	<< false;

		QTest::newRow("ImageGenerator.vuo")	<< "VuoImageFilter"		<< false;
		QTest::newRow("ImageGenerator.vuo")	<< "VuoImageGenerator"	<< true;

		QTest::newRow("NoProtocol.vuo")		<< "VuoImageFilter"		<< false;
		QTest::newRow("NoProtocol.vuo")		<< "VuoImageGenerator"	<< false;
	}
	void testCanOpenComposition(void)
	{
		QFETCH(QString, className);
		QFETCH(bool, expectedCompliance);

		Class c = NSClassFromString([NSString stringWithUTF8String:className.toUtf8().data()]);

		{
			bool actualCompliance = [c canOpenComposition:url()];
			QCOMPARE(actualCompliance, expectedCompliance);
		}

		{
			bool actualCompliance = [c canOpenCompositionString:compositionString()];
			QCOMPARE(actualCompliance, expectedCompliance);
		}
	}

	void testCommonMethods_data()
	{
		QTest::addColumn<QString>("expectedCompositionName");
		QTest::addColumn<QString>("expectedCompositionDescription");
		QTest::addColumn<QString>("expectedCompositionCopyright");
		QTest::addColumn<QList<QString> >("expectedInputPorts");
		QTest::addColumn<QList<QString> >("expectedOutputPorts");
		QTest::addColumn<QList<PortAndDetails> >("expectedPortDetails");
		QTest::addColumn<MapOfStrings>("expectedInputPortValues");
		QTest::addColumn<MapOfStrings>("expectedOutputPortValues");

		QTest::newRow("ImageFilter.vuo")
				<< "ImageFilter"
				<< "An empty composition that complies with the ImageFilter protocol."
				<< "Copyright © 2012–2016 Kosada Incorporated. This code may be modified and distributed under the terms of the MIT License. For more information, see http://vuo.org/license."
				<< QList<QString>()
				<< QList<QString>()
				<< QList<PortAndDetails>()
				<< MapOfStrings()
				<< MapOfStrings();

		QTest::newRow("ImageFilter512.vuo")
				<< "ImageFilter512"
				<< "A large composition that complies with the ImageFilter protocol."
				<< "Copyright © 2012–2016 Kosada Incorporated. This code may be modified and distributed under the terms of the MIT License. For more information, see http://vuo.org/license."
				<< QList<QString>()
				<< QList<QString>()
				<< QList<PortAndDetails>()
				<< MapOfStrings()
				<< MapOfStrings();

		{
			QMap<QString, QString> curveEasingDetails;
			curveEasingDetails["title"] = "curveEasing";
			curveEasingDetails["type"] = "VuoCurveEasing";
			curveEasingDetails["default"] = "in";
			curveEasingDetails["menuItems"] = "In,Out,In + Out,Middle,";

			MapOfStrings inputPortValues;
			inputPortValues["background"] = "{\"r\":1,\"g\":1,\"b\":1,\"a\":1}";
			inputPortValues["curveEasing"] = "\"in\"";
			inputPortValues["foreground"] = "{\"r\":0,\"g\":0,\"b\":0,\"a\":1}";
			inputPortValues["foregroundOpacity"] = "0.5";

			MapOfStrings outputPortValues;
			outputPortValues["blendedColor"] = "0,0,0,0";

			QTest::newRow("ImageFilterWithExtraPorts.vuo")
					<< "ImageFilter with unconnected extra ports"
					<< "An ImageFilter with unconnected non-protocol ports."
					<< "Copyright © 2012–2016 Kosada Incorporated. This code may be modified and distributed under the terms of the MIT License. For more information, see http://vuo.org/license."
					<< (QList<QString>() << "background" << "foreground" << "curveEasing" << "foregroundOpacity")
					<< (QList<QString>() << "blendedColor")
					// Disconnected published ports should have a title, type, default, and menuItems, but no other details.
					<< (QList<PortAndDetails>() << PortAndDetails("curveEasing", curveEasingDetails))
					<< inputPortValues
					<< outputPortValues;
		}

		{
			QMap<QString, QString> curveEasingDetails;
			curveEasingDetails["title"] = "curveEasing";
			curveEasingDetails["type"] = "VuoCurveEasing";
			curveEasingDetails["default"] = "middle"; // Verify that this published port's initial value doesn't match its internal connected port's default.
			curveEasingDetails["menuItems"] = "In,Out,In + Out,Middle,";

			MapOfStrings inputPortValues;
			inputPortValues["background"] = "{\"r\":1,\"g\":1,\"b\":1,\"a\":1}";
			inputPortValues["curveEasing"] = "\"middle\"";
			inputPortValues["foreground"] = "{\"r\":0,\"g\":0,\"b\":0,\"a\":1}";
			inputPortValues["foregroundOpacity"] = "0.5";

			MapOfStrings outputPortValues;
			outputPortValues["blendedColor"] = "0,0,0,0";

			QTest::newRow("ImageFilterWithExtraPortsConnected.vuo")
					<< "ImageFilter with connected extra ports"
					<< "An ImageFilter with connected non-protocol ports."
					<< "Copyright © 2012–2016 Kosada Incorporated. This code may be modified and distributed under the terms of the MIT License. For more information, see http://vuo.org/license."
					<< (QList<QString>() << "background" << "foreground" << "curveEasing" << "foregroundOpacity")
					<< (QList<QString>() << "blendedColor")
					<< (QList<PortAndDetails>() << PortAndDetails("curveEasing", curveEasingDetails))
					<< inputPortValues
					<< outputPortValues;
		}

		{
			QMap<QString, QString> positionDetails;
			positionDetails["title"] = "position";
			positionDetails["type"] = "VuoPoint3d";
			positionDetails["default"] = "0,0,1";
			positionDetails["suggestedMin"] = "0,0,0";
			positionDetails["suggestedMax"] = "360,360,360";
			positionDetails["suggestedStep"] = "0.1,0.1,0.1";

			QMap<QString, QString> fieldOfViewDetails;
			fieldOfViewDetails["title"] = "fieldOfView";
			fieldOfViewDetails["type"] = "VuoReal";
			fieldOfViewDetails["default"] = "0.01";
			fieldOfViewDetails["suggestedMin"] = "0.01";
			fieldOfViewDetails["suggestedMax"] = "179.9";
			fieldOfViewDetails["suggestedStep"] = "1";

			MapOfStrings inputPortValues;
			inputPortValues["fieldOfView"] = "0.01";
			inputPortValues["position"] = "{\"x\":0,\"y\":0,\"z\":1}";

			QTest::newRow("ImageFilterWithDetailCoalescing.vuo")
					<< "ImageFilter with Detail Coalescing"
					<< "An ImageFilter with published input ports connected to multiple internal ports each with differing details."
					<< "Copyright © 2012–2016 Kosada Incorporated. This code may be modified and distributed under the terms of the MIT License. For more information, see http://vuo.org/license."
					<< (QList<QString>() << "position" << "fieldOfView")
					<< QList<QString>()
					<< (QList<PortAndDetails>() << PortAndDetails("position", positionDetails) << PortAndDetails("fieldOfView", fieldOfViewDetails))
					<< inputPortValues
					<< MapOfStrings();
		}
	}
	void testCommonMethods(void)
	{
		QFETCH(QString, expectedCompositionName);
		QFETCH(QString, expectedCompositionDescription);
		QFETCH(QString, expectedCompositionCopyright);
		QFETCH(QList<QString>, expectedInputPorts);
		QFETCH(QList<QString>, expectedOutputPorts);
		QFETCH(QList<PortAndDetails>, expectedPortDetails);
		QFETCH(MapOfStrings, expectedInputPortValues);
		QFETCH(MapOfStrings, expectedOutputPortValues);

		for (int pass = 0; pass < 2; ++pass)
		{
			VuoImageFilter *vif;
			if (pass == 0)
				vif = [[VuoImageFilter alloc] initWithComposition:url()];
			else
				vif = [[VuoImageFilter alloc] initWithCompositionString:compositionString() name:[NSString stringWithUTF8String:QTest::currentDataTag()] sourcePath:@"composition"];
			QVERIFY(vif);
			activeRunner = vif;

			QCOMPARE(QString([[vif compositionName]        UTF8String]), expectedCompositionName);
			QCOMPARE(QString([[vif compositionDescription] UTF8String]), expectedCompositionDescription);
			QCOMPARE(QString([[vif compositionCopyright]   UTF8String]), expectedCompositionCopyright);

			QCOMPARE(qListQStringWithNSArray([vif inputPorts]),  expectedInputPorts);
			QCOMPARE(qListQStringWithNSArray([vif outputPorts]), expectedOutputPorts);

			foreach (PortAndDetails pd, expectedPortDetails)
				QCOMPARE(qMapQStringWithNSDictionary([vif detailsForPort:nsStringWithQString(pd.first)]), pd.second);

			id propertyList = [vif propertyListFromInputValues];
			QCOMPARE(qMapQStringWithNSDictionary(propertyList), expectedInputPortValues);

			bool successfullySetPropertyList = [vif setInputValuesWithPropertyList:propertyList];
			// If the property list has values, and any of them are null, we expect `setInputValuesWithPropertyList:` to fail.
			bool areAllInputsValid = expectedInputPortValues.empty() || expectedInputPortValues.first() != "null";
			QCOMPARE(successfullySetPropertyList, areAllInputsValid);

			QMapIterator<QString, QString> i(expectedOutputPortValues);
			while (i.hasNext())
			{
				i.next();
				QCOMPARE(QString([[[vif valueForOutputPort:[NSString stringWithUTF8String:i.key().toUtf8().data()]] stringValue] UTF8String]), i.value());
			}

			bool successfullySetValue = [vif setValue:[NSNumber numberWithFloat:42] forInputPort:@"time"];
			QVERIFY(successfullySetValue);

			[vif release];
			activeRunner = nil;
		}
	}

	void testDataTypes_data()
	{
		QTest::addColumn<void *>("data");

		QTest::newRow("boolean")	<< (void *)[NSNumber numberWithBool:YES];
		QTest::newRow("integer")	<< (void *)[NSNumber numberWithInt:-42];
		QTest::newRow("real")		<< (void *)[NSNumber numberWithFloat:.42];
		QTest::newRow("curve")		<< (void *)@"exponential";
		QTest::newRow("curveList")	<< (void *)[NSArray arrayWithObjects:@"circular", @"quadratic", nil];
		QTest::newRow("text")		<< (void *)@"流";
		QTest::newRow("color")		<< (void *)blueishColor;
		QTest::newRow("color")		<< (void *)[CIColor colorWithRed:0.1 green:0.2 blue:0.3 alpha:0.4];
		QTest::newRow("point2d")	<< (void *)[NSValue valueWithPoint:NSMakePoint(4,2)];
		QTest::newRow("point2d")	<< (void *)[NSValue valueWithPoint:CGPointMake(4,2)];
		QTest::newRow("point3d")	<< (void *)[NSData dataWithBytes:(double[3]){42,4,2} length:sizeof(double[3])];
	}
	void testDataTypes(void)
	{
		QFETCH(void *, data);
		id inputData = (id)data;

		bool valueSetSuccessfully = [sharedPassthruImageFilter setValue:inputData forInputPort:[NSString stringWithUTF8String:QTest::currentDataTag()]];
		QVERIFY(valueSetSuccessfully);

		NSImage *outputImage = [sharedPassthruImageFilter filterNSImage:reddishImage atTime:0];
		QVERIFY(outputImage);

		QString inputPortName = QString(QTest::currentDataTag());
		QString outputPortName = QString("output") + inputPortName[0].toUpper() + inputPortName.mid(1);
		id outputData = [sharedPassthruImageFilter valueForOutputPort:[NSString stringWithUTF8String:outputPortName.toUtf8().data()]];
//		NSLog(@" inputData: %@", inputData);
//		NSLog(@"outputData: %@", outputData);

		if ([inputData respondsToSelector:@selector(objCType)] && strcmp([inputData objCType], "f") == 0)
			// If it's an NSNumber that contains a float, use fuzzy comparison.
			QVERIFY(fabs([inputData floatValue] - [outputData floatValue]) < 0.00001);
		else if ([inputData isKindOfClass:[CIColor class]])
		{
			// If it's a CIColor, convert to NSColor for comparison.
			NSColor *inputNSColor = [[NSColor colorWithCIColor:inputData] colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
			// Compare descriptions rather than values, since comparing values always seems to return NO.
			// The description includes the colorspace and color values, so this should be good enough.
			QVERIFY([[inputNSColor description] isEqualTo:[outputData description]]);
		}
		else if ([inputData respondsToSelector:@selector(objectAtIndex:)])
			// If we're inputting an NSArray, just compare the first item
			// (since the Share Value passthru node doesn't support lists at the time of writing).
			QVERIFY([[inputData objectAtIndex:0] isEqualTo:outputData]);
		else
			QVERIFY([outputData isEqualTo:inputData]);
	}

	void testImageFilterNSImage(void)
	{
		// Send the image through the VuoImageFilter.
		NSImage *outputNSImage = [sharedImageFilter filterNSImage:reddishImage atTime:0];
		QVERIFY(outputNSImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE([outputNSImage size].width,  10.f);
		QCOMPARE([outputNSImage size].height, 10.f);

		// Check pixel (2,2).
		NSBitmapImageRep *outputBIR = [NSBitmapImageRep imageRepWithData:[outputNSImage TIFFRepresentation]];
		QVERIFY(outputBIR);
		NSColor *color = [outputBIR colorAtX:2 y:2];
		QVERIFY(fabs([color redComponent]   - 1          ) < 0.00001);
		QVERIFY(fabs([color greenComponent] - 127.f/255.f) < 0.00001);
		QVERIFY(fabs([color blueComponent]  - 127.f/255.f) < 0.00001);
		QVERIFY(fabs([color alphaComponent] - 1          ) < 0.00001);
	}

	void testImageFilterNSBitmapImageRep(void)
	{
		NSBitmapImageRep *bir = [[reddishImage representations] objectAtIndex:0];

		bool valueSetSuccessfully = [sharedPassthruImageFilter setValue:bir forInputPort:@"image2"];
		QVERIFY(valueSetSuccessfully);

		NSImage *outputImage = [sharedPassthruImageFilter filterNSImage:reddishImage atTime:0];
		QVERIFY(outputImage);

		NSImage *outputImage2 = [sharedPassthruImageFilter valueForOutputPort:@"outputImage2"];
		QVERIFY(outputImage2);

		QVERIFY([[outputImage2 TIFFRepresentation] isEqualToData:[reddishImage TIFFRepresentation]]);
	}

	void testImageFilterCGImageRef(void)
	{
		CGImageSourceRef cgImageSource = CGImageSourceCreateWithData((CFDataRef)[reddishImage TIFFRepresentation], NULL);
		CGImageRef inputCGImage =  CGImageSourceCreateImageAtIndex(cgImageSource, 0, NULL);

		bool valueSetSuccessfully = [sharedPassthruImageFilter setValue:(id)inputCGImage forInputPort:@"image2"];
		QVERIFY(valueSetSuccessfully);

		NSImage *outputImage = [sharedPassthruImageFilter filterNSImage:reddishImage atTime:0];
		QVERIFY(outputImage);

		NSImage *outputImage2 = [sharedPassthruImageFilter valueForOutputPort:@"outputImage2"];
		QVERIFY(outputImage2);

		QVERIFY([[outputImage2 TIFFRepresentation] isEqualToData:[reddishImage TIFFRepresentation]]);

		CGImageRelease(inputCGImage);
		CFRelease(cgImageSource);
	}

	void testImageFilterCVPixelBufferRef(void)
	{
		// Create a CVPixelBufferRef.
		CVPixelBufferRef inputCVPB;
		{
			NSBitmapImageRep *bir = [[reddishImage representations] objectAtIndex:0];
			QVERIFY(bir);
			// [bir bitmapData] is in RGBA format, but the CVPixelBufferRef expects BGRA.  Account for this later...
			CVReturn createdSuccessfully = CVPixelBufferCreateWithBytes(NULL, 10, 10, kCVPixelFormatType_32BGRA, [bir bitmapData], [bir bytesPerRow], cvPixelBufferFreeCallback, NULL, nil, &inputCVPB);
			QCOMPARE((long)kCVReturnSuccess, (long)createdSuccessfully);
		}

		// Send the image through the VuoImageFilter.
		NSImage *outputNSImage = [sharedImageFilter filterNSImage:(id)inputCVPB atTime:0];
		QVERIFY(outputNSImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE([outputNSImage size].width,  10.f);
		QCOMPARE([outputNSImage size].height, 10.f);

		// Check pixel (5,5).
		NSBitmapImageRep *outputBIR = [NSBitmapImageRep imageRepWithData:[outputNSImage TIFFRepresentation]];
		QVERIFY(outputBIR);
		NSColor *color = [outputBIR colorAtX:5 y:5];
		// Though we're using reddishImage, the R and B channels are swapped above.
		QVERIFY(fabs([color redComponent]   - 127.f/255.f) < 0.00001);
		QVERIFY(fabs([color greenComponent] - 127.f/255.f) < 0.00001);
		QVERIFY(fabs([color blueComponent]  - 1          ) < 0.00001);
		QVERIFY(fabs([color alphaComponent] - 1          ) < 0.00001);
	}

	void testImageFilterCVOpenGLTexture(void)
	{
		// Create a CVPixelBufferRef.
		CVPixelBufferRef inputCVPB;
		{
			NSBitmapImageRep *bir = [[reddishImage representations] objectAtIndex:0];
			QVERIFY(bir);

			// [bir bitmapData] is in RGBA format, but the CVPixelBufferRef expects BGRA.
			// [bir bitmapData] is rightsideup, but the CVOpenGLTexture expects flipped.
			// Translate both.
			char *bitmapData = (char *)[bir bitmapData];
			int bytesPerRow = [bir bytesPerRow];
			int width = [reddishImage size].width;
			int height = [reddishImage size].height;
			char *flippedBitmapData = (char *)malloc(bytesPerRow*height);
			for (unsigned long y = 0; y < height; ++y)
				for (unsigned long x = 0; x < width; ++x)
				{
					flippedBitmapData[bytesPerRow * (height - y - 1) + x * 4 + 2] = bitmapData[bytesPerRow * y + x * 4 + 0];
					flippedBitmapData[bytesPerRow * (height - y - 1) + x * 4 + 1] = bitmapData[bytesPerRow * y + x * 4 + 1];
					flippedBitmapData[bytesPerRow * (height - y - 1) + x * 4 + 0] = bitmapData[bytesPerRow * y + x * 4 + 2];
					flippedBitmapData[bytesPerRow * (height - y - 1) + x * 4 + 3] = bitmapData[bytesPerRow * y + x * 4 + 3];
				}

			CVReturn createdSuccessfully = CVPixelBufferCreateWithBytes(NULL, 10, 10, kCVPixelFormatType_32BGRA, flippedBitmapData, bytesPerRow, cvPixelBufferFreeCallback, NULL, nil, &inputCVPB);
			QCOMPARE((long)kCVReturnSuccess, (long)createdSuccessfully);
		}

		// Create a CVOpenGLTexture.
		CVOpenGLTextureCacheRef textureCache;
		CVOpenGLTextureRef inputCVOGLT;
		{
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			CGLPixelFormatObj pf = (CGLPixelFormatObj)VuoGlContext_makePlatformPixelFormat(false);
			CVReturn ret = CVOpenGLTextureCacheCreate(NULL, NULL, cgl_ctx, pf, NULL, &textureCache);
			CGLReleasePixelFormat(pf);
			QVERIFY(ret == kCVReturnSuccess);

			ret = CVOpenGLTextureCacheCreateTextureFromImage(NULL, textureCache, inputCVPB, NULL, &inputCVOGLT);
			QVERIFY(ret == kCVReturnSuccess);
		}

		bool valueSetSuccessfully = [sharedPassthruImageFilter setValue:(id)inputCVOGLT forInputPort:@"image2"];
		QVERIFY(valueSetSuccessfully);

		NSImage *outputImage = [sharedPassthruImageFilter filterNSImage:reddishImage atTime:0];
		QVERIFY(outputImage);

		NSImage *outputImage2 = [sharedPassthruImageFilter valueForOutputPort:@"outputImage2"];
		QVERIFY(outputImage2);

		QVERIFY([[outputImage2 TIFFRepresentation] isEqualToData:[reddishImage TIFFRepresentation]]);

		CVPixelBufferRelease(inputCVPB);
		CVOpenGLTextureRelease(inputCVOGLT);
		CVOpenGLTextureCacheRelease(textureCache);
	}

	void testImageFilterGLTexture2D(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Send the image through the VuoImageFilter.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
		QVERIFY(outputGLTexture);
		VuoRelease(inputVuoImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_make(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage);
	}

	void testImageFilterGLTexture2DAlpha(void)
	{
		// Create a semitransparent reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,0.5), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Send the image through the VuoImageFilter.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
		QVERIFY(outputGLTexture);
		VuoRelease(inputVuoImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_make(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 127) < 2);
		VuoRelease(outputVuoImage);
	}

	void testImageFilterGLTextureRectangle(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Convert to GL_TEXTURE_RECTANGLE_ARB.
		VuoImage inputVuoImageRectangle = VuoImage_makeGlTextureRectangleCopy(inputVuoImage);
		VuoRetain(inputVuoImageRectangle);
		VuoRelease(inputVuoImage);

		// Send the image through the VuoImageFilter.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImageRectangle->glTextureName target:GL_TEXTURE_RECTANGLE_ARB pixelsWide:inputVuoImageRectangle->pixelsWide pixelsHigh:inputVuoImageRectangle->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
		QVERIFY(outputGLTexture);
		VuoRelease(inputVuoImageRectangle);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage);
	}

	void testImageFilterGLTextureRectangleProvider(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Create an output texture.
		GLuint outputGLTexture;
		{
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			glGenTextures(1, &outputGLTexture);
			// Do not call glTexImage2D() on this texture, since that makes CGLTexImageIOSurface2D() throw an "invalid context state" error.
			VuoGlContext_disuse(cgl_ctx);
		}
		QVERIFY(outputGLTexture);
		GLuint (^provider)(NSUInteger, NSUInteger) = ^GLuint(NSUInteger pixelsWide, NSUInteger pixelsHigh){
			return outputGLTexture;
		};

		// Send the image through the VuoImageFilter.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		IOSurfaceRef outputIOSurface;
		GLuint actualOutputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 withTextureProvider:provider outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh  ioSurface:&outputIOSurface];
		QVERIFY(actualOutputGLTexture);
		QCOMPARE(actualOutputGLTexture, outputGLTexture);
		VuoRelease(inputVuoImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		QVERIFY(outputIOSurface);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);

		// Confirm that Radar #23547737 still hasn't been fixed.
		QVERIFY(!pixels);

		// Workaround for Radar #23547737:
		VuoImage outputVuoImage2 = VuoImage_makeCopy(outputVuoImage, false);
		QVERIFY(outputVuoImage2);
		VuoRetain(outputVuoImage2);
		VuoRelease(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage2, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage2);

		VuoIoSurfacePool_signal(outputIOSurface);
		CFRelease(outputIOSurface);


		// At this point, the host app is done with the texture.
		// Let's reuse it to filter another image.


		// Create a greenish OpenGL texture.
		inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(0.5,1,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Send the image through the VuoImageFilter.
		actualOutputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 withTextureProvider:provider outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh ioSurface:&outputIOSurface];
		QVERIFY(actualOutputGLTexture);
		QVERIFY(outputIOSurface);
		QCOMPARE(actualOutputGLTexture, outputGLTexture);
		VuoRelease(inputVuoImage);

		// ImageFilters aren't required to output an image with the same width/height as the input image,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);

		// Confirm that Radar #23547737 still hasn't been fixed.
		QVERIFY(!pixels);

		// Workaround for Radar #23547737:
		outputVuoImage2 = VuoImage_makeCopy(outputVuoImage, false);
		QVERIFY(outputVuoImage2);
		VuoRetain(outputVuoImage2);
		VuoRelease(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage2, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage2);

		VuoIoSurfacePool_signal(outputIOSurface);
		CFRelease(outputIOSurface);
	}

	void testImageGeneratorNSImage(void)
	{
		// Choose a color
		[sharedImageGenerator setValue:blueishColor forInputPort:@"color"];

		// Request an image.
		NSImage *outputNSImage = [sharedImageGenerator generateNSImageWithSuggestedPixelsWide:10 pixelsHigh:10 atTime:0];
		QVERIFY(outputNSImage);

		// ImageGenerators aren't required to output the same width and height as input,
		// but this particular ImageGeneratorColor.vuo composition does.
		QCOMPARE([outputNSImage size].width,  10.f);
		QCOMPARE([outputNSImage size].height, 10.f);

		// Check pixel (5,5).
		NSBitmapImageRep *outputBIR = [NSBitmapImageRep imageRepWithData:[outputNSImage TIFFRepresentation]];
		QVERIFY(outputBIR);
		NSColor *color = [outputBIR colorAtX:5 y:5];
		QVERIFY(fabs([color redComponent]   - 127.f/255.f) < 0.01);
		QVERIFY(fabs([color greenComponent] - 127.f/255.f) < 0.01);
		QVERIFY(fabs([color blueComponent]  - 1          ) < 0.01);
		QVERIFY(fabs([color alphaComponent] - 1          ) < 0.01);
	}

	void testImageGeneratorGLTexture2D(void)
	{
		// Choose a color
		[sharedImageGenerator setValue:blueishColor forInputPort:@"color"];

		// Request an image.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		GLuint outputGLTexture = [sharedImageGenerator generateGLTextureWithTarget:GL_TEXTURE_2D suggestedPixelsWide:10 pixelsHigh:10 atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
		QVERIFY(outputGLTexture);

		// ImageGenerators aren't required to output the same width and height as input,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_make(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the red/green values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage);
	}

	void testImageGeneratorGLTextureRectangle(void)
	{
		// Choose a color
		[sharedImageGenerator setValue:blueishColor forInputPort:@"color"];

		// Request an image.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		GLuint outputGLTexture = [sharedImageGenerator generateGLTextureWithTarget:GL_TEXTURE_RECTANGLE_ARB suggestedPixelsWide:10 pixelsHigh:10 atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
		QVERIFY(outputGLTexture);

		// ImageGenerators aren't required to output the same width and height as input,
		// but this particular ImageFilterRipple.vuo composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the red/green values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage);
	}

	void testImageGeneratorGLTextureRectangleProvider(void)
	{
		// Choose a color
		[sharedImageGenerator setValue:blueishColor forInputPort:@"color"];

		// Create an output texture.
		GLuint outputGLTexture;
		{
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			glGenTextures(1, &outputGLTexture);
			// Do not call glTexImage2D() on this texture, since that makes CGLTexImageIOSurface2D() throw an "invalid context state" error.
			VuoGlContext_disuse(cgl_ctx);
		}
		QVERIFY(outputGLTexture);
		GLuint (^provider)(NSUInteger, NSUInteger) = ^GLuint(NSUInteger pixelsWide, NSUInteger pixelsHigh){
			return outputGLTexture;
		};

		// Request an image.
		NSUInteger outputPixelsWide, outputPixelsHigh;
		IOSurfaceRef outputIOSurface;
		GLuint actualOutputGLTexture = [sharedImageGenerator generateGLTextureWithProvider:provider suggestedPixelsWide:10 pixelsHigh:10 atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh ioSurface:&outputIOSurface];
		QVERIFY(actualOutputGLTexture);
		QCOMPARE(actualOutputGLTexture, outputGLTexture);

		// ImageGenerators aren't required to output an image with the same width/height as the input image,
		// but this particular composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		VuoImage outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		const unsigned char *pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);

		// Confirm that Radar #23547737 still hasn't been fixed.
		QVERIFY(!pixels);

		// Workaround for Radar #23547737:
		VuoImage outputVuoImage2 = VuoImage_makeCopy(outputVuoImage, false);
		QVERIFY(outputVuoImage2);
		VuoRetain(outputVuoImage2);
		VuoRelease(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage2, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage2);

		VuoIoSurfacePool_signal(outputIOSurface);
		CFRelease(outputIOSurface);


		// At this point, the host app is done with the texture.
		// Let's reuse it to filter another image.


		// Choose a color
		[sharedImageGenerator setValue:greenishColor forInputPort:@"color"];

		// Request an image.
		actualOutputGLTexture = [sharedImageGenerator generateGLTextureWithProvider:provider suggestedPixelsWide:10 pixelsHigh:10 atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh ioSurface:&outputIOSurface];
		QVERIFY(actualOutputGLTexture);
		QCOMPARE(actualOutputGLTexture, outputGLTexture);

		// ImageGenerators aren't required to output an image with the same width/height as the input image,
		// but this particular composition does.
		QCOMPARE(outputPixelsWide, (unsigned long)10);
		QCOMPARE(outputPixelsHigh, (unsigned long)10);

		// Fetch the pixel buffer.
		outputVuoImage = VuoImage_makeClientOwnedGlTextureRectangle(outputGLTexture, GL_RGBA, outputPixelsWide, outputPixelsHigh, freeCallback, NULL);
		QVERIFY(outputVuoImage);
		VuoRetain(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage, GL_BGRA);

		// Confirm that Radar #23547737 still hasn't been fixed.
		QVERIFY(!pixels);

		// Workaround for Radar #23547737:
		outputVuoImage2 = VuoImage_makeCopy(outputVuoImage, false);
		QVERIFY(outputVuoImage2);
		VuoRetain(outputVuoImage2);
		VuoRelease(outputVuoImage);
		pixels = VuoImage_getBuffer(outputVuoImage2, GL_BGRA);
		QVERIFY(pixels);

		// Check pixel (5,5).
		// For unknown reasons the green/blue values fluctuate between 126 and 127, so use a tolerance of 2.
		QVERIFY(abs(pixels[(10*5*4)+5*4+2] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+1] - 255) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+0] - 127) < 2);
		QVERIFY(abs(pixels[(10*5*4)+5*4+3] - 255) < 2);
		VuoRelease(outputVuoImage2);

		VuoIoSurfacePool_signal(outputIOSurface);
		CFRelease(outputIOSurface);
	}

	void testGLTexture2DLeaksInHostApp(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Send the image through the VuoImageFilter a bunch of times.
		QSet<GLuint> uniqueTextureNames;
		{
			NSUInteger outputPixelsWide, outputPixelsHigh;
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			for (int i = 0; i < 1000; ++i)
			{
				GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
				QVERIFY(outputGLTexture);
				uniqueTextureNames.insert(outputGLTexture);
				glDeleteTextures(1, &outputGLTexture);
			}
			VuoGlContext_disuse(cgl_ctx);
		}

		// Make sure we're using a reasonable number of textures.
		VUserLog("%d unique textures",uniqueTextureNames.size());
		QVERIFY(uniqueTextureNames.size() < 50);

		VuoRelease(inputVuoImage);
	}

	void testGLTextureRectangleLeaksInHostApp(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Convert to GL_TEXTURE_RECTANGLE_ARB.
		VuoImage inputVuoImageRectangle = VuoImage_makeGlTextureRectangleCopy(inputVuoImage);
		VuoRetain(inputVuoImageRectangle);
		VuoRelease(inputVuoImage);

		// Send the image through the VuoImageFilter a bunch of times.
		QSet<GLuint> uniqueTextureNames;
		{
			NSUInteger outputPixelsWide, outputPixelsHigh;
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			for (int i = 0; i < 1000; ++i)
			{
				GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImageRectangle->glTextureName target:GL_TEXTURE_RECTANGLE_ARB pixelsWide:inputVuoImageRectangle->pixelsWide pixelsHigh:inputVuoImageRectangle->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
				QVERIFY(outputGLTexture);
				uniqueTextureNames.insert(outputGLTexture);
				glDeleteTextures(1, &outputGLTexture);
			}
			VuoGlContext_disuse(cgl_ctx);
		}

		// Make sure we're using a reasonable number of textures.
		VUserLog("%d unique textures",uniqueTextureNames.size());
		QVERIFY(uniqueTextureNames.size() < 50);

		VuoRelease(inputVuoImageRectangle);
	}

	void testImageFilterGLTexture2DPerformance(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Wait for the VuoImageFilter to finish compiling.
		[sharedImageFilter inputPorts];

		// Send the image through the VuoImageFilter a bunch of times.
		{
			NSUInteger outputPixelsWide, outputPixelsHigh;
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			QBENCHMARK {
				GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImage->glTextureName target:GL_TEXTURE_2D pixelsWide:inputVuoImage->pixelsWide pixelsHigh:inputVuoImage->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
				QVERIFY(outputGLTexture);
				glDeleteTextures(1, &outputGLTexture);
			}
			VuoGlContext_disuse(cgl_ctx);
		}

		VuoRelease(inputVuoImage);
	}

	void testImageFilterGLTextureRectanglePerformance(void)
	{
		// Create a reddish OpenGL texture.
		VuoImage inputVuoImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), 10, 10);
		QVERIFY(inputVuoImage);
		VuoRetain(inputVuoImage);

		// Convert to GL_TEXTURE_RECTANGLE_ARB.
		VuoImage inputVuoImageRectangle = VuoImage_makeGlTextureRectangleCopy(inputVuoImage);
		VuoRetain(inputVuoImageRectangle);
		VuoRelease(inputVuoImage);

		// Wait for the VuoImageFilter to finish compiling.
		[sharedImageFilter inputPorts];

		// Send the image through the VuoImageFilter a bunch of times.
		{
			NSUInteger outputPixelsWide, outputPixelsHigh;
			CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();
			QBENCHMARK {
				GLuint outputGLTexture = [sharedImageFilter filterGLTexture:inputVuoImageRectangle->glTextureName target:GL_TEXTURE_RECTANGLE_ARB pixelsWide:inputVuoImageRectangle->pixelsWide pixelsHigh:inputVuoImageRectangle->pixelsHigh atTime:0 outputPixelsWide:&outputPixelsWide pixelsHigh:&outputPixelsHigh];
				QVERIFY(outputGLTexture);
				glDeleteTextures(1, &outputGLTexture);
			}
			VuoGlContext_disuse(cgl_ctx);
		}

		VuoRelease(inputVuoImageRectangle);
	}

	void cleanup(void)
	{
		// Called after every test function.
		// If a test fails, the test function returns immediately without cleaning up, which causes the process to hang.  Prevent that.
		if (activeRunner)
		{
			[activeRunner release];
			activeRunner = nil;
		}
	}

	void cleanupTestCase(void)
	{
		[sharedImageFilter release];
		[sharedPassthruImageFilter release];
		[sharedImageGenerator release];
		[reddishImage release];
	}
};

QTEST_APPLESS_MAIN(TestVuoRunnerCocoa)
#include "TestVuoRunnerCocoa.moc"
