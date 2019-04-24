/**
 * @file
 * TestVuoImage implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoImage.h"
#include "VuoShader.h"
#include "VuoImageGet.h"
#include "VuoImageRenderer.h"
#include "VuoImageBlur.h"
#include "VuoSceneRenderer.h"
}

#include <OpenGL/CGLMacro.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoColor);
Q_DECLARE_METATYPE(VuoImage);

bool TestVuoImage_freed = false;
void TestVuoImage_freeCallback(VuoImage imageToFree)
{
	TestVuoImage_freed = true;
}

/**
 * Tests the VuoImage type.
 */
class TestVuoImage : public QObject
{
	Q_OBJECT

	const char *solidColorVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
		uniform mat4 projectionMatrix;
		uniform mat4 modelviewMatrix;
		attribute vec4 position;

		void main()
		{
			gl_Position = projectionMatrix * modelviewMatrix * position;
		}
	);

	const char *solidColorFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		void main()
		{
			gl_FragColor = vec4(1.,1.,1.,1.);
		}
	);

	const char *rippleFragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
		// Inputs
		uniform sampler2D texture;
		uniform float angle;
		uniform float amplitude;
		uniform float wavelength;
		uniform float phase;
		varying vec4 fragmentTextureCoordinate;

		void main()
		{
			float samplerPhase = cos(angle)*fragmentTextureCoordinate.x + sin(angle)*fragmentTextureCoordinate.y;
			float offset = sin(samplerPhase/wavelength + phase) * amplitude;
			gl_FragColor = texture2D(texture, fragmentTextureCoordinate.xy + vec2(cos(angle)*offset,sin(angle)*offset));
		}
	);

private slots:

	void testNull()
	{
		QCOMPARE(QString::fromUtf8(VuoImage_getString(NULL)), QString("null"));
		QCOMPARE(QString::fromUtf8(VuoImage_getInterprocessString(NULL)), QString("null"));
		QCOMPARE(QString::fromUtf8(VuoImage_getSummary(NULL)), QString("(no image)"));
	}

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("valid"); // Is @c value expected to produce a valid result?
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
										<< false
										<< "";

		QTest::newRow("make")			<< QString(VuoImage_getString(VuoImage_make(43,GL_RGBA,640,480)))
										<< true
										<< "GL Texture (ID 43)<br>Size: 640x480 pixels<br>Scale Factor: 1x<br>Type: RGBA, each channel stored as 8-bit unsigned integer (GL_RGBA)";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, valid);
		QFETCH(QString, summary);

		VuoImage t = VuoImage_makeFromString(value.toUtf8().data());
		if (valid)
		{
			QCOMPARE(QString::fromUtf8(VuoImage_getString(t)), value);
			QCOMPARE(QString::fromUtf8(VuoImage_getSummary(t)), summary);
		}
	}

	void testJsonColorImage()
	{
		VuoImage image = VuoImage_makeFromString(QUOTE({"color":{"r":0.5,"g":1,"b":0,"a":1},"pixelsWide":640,"pixelsHigh":480}));
		QVERIFY(image);

		VuoRetain(image);
		const unsigned char *imageBuffer = VuoImage_getBuffer(image, GL_BGRA);
		QVERIFY(abs(imageBuffer[2] - 127) < 2);
		QVERIFY(abs(imageBuffer[1] - 255) < 2);
		QVERIFY(abs(imageBuffer[0] -   0) < 2);
		QVERIFY(abs(imageBuffer[3] - 255) < 2);
		VuoRelease(image);
	}

	void testImageEquality_data()
	{
		QTest::addColumn<VuoImage>("a");
		QTest::addColumn<VuoImage>("b");
		QTest::addColumn<bool>("expectedEquality");
		QTest::addColumn<bool>("expectedAEmpty");
		QTest::addColumn<bool>("expectedBEmpty");

		QTest::newRow("null null")			<< (VuoImage)NULL
											<< (VuoImage)NULL
											<< true
											<< true
											<< true;

		QTest::newRow("null nonnull")		<< (VuoImage)NULL
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< false
											<< true
											<< false;

		QTest::newRow("nonnull null")		<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< (VuoImage)NULL
											<< false
											<< false
											<< true;

		QTest::newRow("same color")			<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< true
											<< false
											<< false;

		QTest::newRow("different alpha")	<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,0),1,1)
											<< false
											<< false
											<< true;

		QTest::newRow("different color")	<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,.1),1,1)
											<< false
											<< false
											<< false;
	}
	void testImageEquality()
	{
		QFETCH(VuoImage, a);
		QFETCH(VuoImage, b);
		QFETCH(bool, expectedEquality);
		QFETCH(bool, expectedAEmpty);
		QFETCH(bool, expectedBEmpty);

		bool actualEquality = VuoImage_areEqual(a,b);
		QCOMPARE(actualEquality, expectedEquality);

		bool actualAEmpty = VuoImage_isEmpty(a);
		QCOMPARE(actualAEmpty, expectedAEmpty);

		bool actualBEmpty = VuoImage_isEmpty(b);
		QCOMPARE(actualBEmpty, expectedBEmpty);
	}

	void testFreeingClientOwned()
	{
		unsigned int glTextureName = 44;	// For this test, it doesn't matter whether it's a valid GL texture.
											// Should be different than the GL texture name used in testSerializationAndSummary, since that one never gets freed.

		TestVuoImage_freed = false;
		VuoImage vi = VuoImage_makeClientOwned(glTextureName, GL_RGBA, 1, 1, TestVuoImage_freeCallback, NULL);
		VuoRetain(vi);
		VuoRelease(vi);
		QVERIFY(TestVuoImage_freed);
	}

	void testFreeingClientOwnedReconstituted()
	{
		unsigned int glTextureName = 45;	// For this test, it doesn't matter whether it's a valid GL texture.
											// Should be different than the GL texture name used in testSerializationAndSummary, since that one never gets freed.

		TestVuoImage_freed = false;
		VuoImage vi = VuoImage_makeClientOwned(glTextureName, GL_RGBA, 1, 1, TestVuoImage_freeCallback, NULL);
		VuoRetain(vi);

		VuoImage vi2 = VuoImage_makeFromJson(VuoImage_getJson(vi));
		VuoRetain(vi2);

		VuoRelease(vi);
		QVERIFY(!TestVuoImage_freed);

		VuoRelease(vi2);
		QVERIFY(TestVuoImage_freed);
	}

	void testFetchImagePerformance()
	{
		QBENCHMARK {
			VuoImage i = VuoImage_get("/Library/Desktop Pictures/Zebras.jpg");
			QVERIFY(i);
			VuoRetain(i);
			QCOMPARE(i->pixelsWide, 5120UL);
			QCOMPARE(i->pixelsHigh, 2880UL);
			VuoRelease(i);
		}
	}

	void testMakeFromBufferPerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);

		QBENCHMARK {
			VuoImage i = VuoImage_makeFromBuffer(buffer, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void*){ /* Wait to free until after the benchmark. */ });
			VuoRetain(i);
			VuoRelease(i);
		}

		free(buffer);
	}

/*
	void testIOSurfacePerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);
		VuoImage localImage = VuoImage_makeFromBuffer(buffer, GL_BGRA, width, height);
		free(buffer);
		VuoRetain(localImage);

		QBENCHMARK {
			json_object *interprocessImageJson = VuoImage_getInterprocessJson(localImage);
			VuoImage remoteImage = VuoImage_makeFromJson(interprocessImageJson);
			VuoRetain(remoteImage);
			VuoRelease(remoteImage);
		}

		VuoRelease(localImage);
	}
*/

	void testImageRendererPerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;

		VuoShader s = VuoShader_make("Solid Color Shader");
		VuoShader_addSource(s, VuoMesh_IndividualTriangles, solidColorVertexShaderSource, NULL, solidColorFragmentShaderSource);
		VuoRetain(s);

		VuoImage i = VuoImageRenderer_render(s, width, height, VuoImageColorDepth_8);
		VuoRetain(i);
		const unsigned char *imageBuffer = VuoImage_getBuffer(i, GL_BGRA);
		QCOMPARE(imageBuffer[0], (unsigned char)255);
		QCOMPARE(imageBuffer[1], (unsigned char)255);
		QCOMPARE(imageBuffer[2], (unsigned char)255);
		QCOMPARE(imageBuffer[3], (unsigned char)255);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_render(s, width, height, VuoImageColorDepth_8);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
	}

	void testImageRendererPerformance16bpc()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;

		VuoShader s = VuoShader_make("Solid Color Shader");
		VuoShader_addSource(s, VuoMesh_IndividualTriangles, solidColorVertexShaderSource, NULL, solidColorFragmentShaderSource);
		VuoRetain(s);

		VuoImage i = VuoImageRenderer_render(s, width, height, VuoImageColorDepth_16);
		VuoRetain(i);
		const unsigned char *imageBuffer = VuoImage_getBuffer(i, GL_BGRA);
		QCOMPARE(imageBuffer[0], (unsigned char)255);
		QCOMPARE(imageBuffer[1], (unsigned char)255);
		QCOMPARE(imageBuffer[2], (unsigned char)255);
		QCOMPARE(imageBuffer[3], (unsigned char)255);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_render(s, width, height, VuoImageColorDepth_16);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
	}

	void testImageRendererRipplePerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
		VuoRetain(sourceImage);

		VuoShader s = VuoShader_make("Ripple Shader");
		VuoShader_addSource(s, VuoMesh_IndividualTriangles, NULL, NULL,rippleFragmentShaderSource);
		VuoRetain(s);

		// Feed parameters to the shader.
		VuoShader_setUniform_VuoImage(s, "texture", sourceImage);
		VuoShader_setUniform_VuoReal(s, "angle", 135.*M_PI/180.);
		VuoShader_setUniform_VuoReal(s, "amplitude", .1);
		VuoShader_setUniform_VuoReal(s, "wavelength", 0.05*M_PI*2.);
		VuoShader_setUniform_VuoReal(s, "phase", 0.*M_PI*2.);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_render(s, width, height, VuoImageColorDepth_8);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
		VuoRelease(sourceImage);
	}


	void testImageBlur1Performance()
	{
		unsigned int width = 640;
		unsigned int height = 480;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
		VuoRetain(sourceImage);

		VuoImageBlur ib = VuoImageBlur_make();
		VuoRetain(ib);

		QBENCHMARK {
			VuoImage i = VuoImageBlur_blur(ib, sourceImage, NULL, VuoBlurShape_Gaussian, 1, 1, FALSE);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(sourceImage);
		VuoRelease(ib);
	}

	void testImageCopy_data()
	{
		QTest::addColumn<VuoColor>("color");

		QTest::newRow("opaque white")			<< VuoColor_makeWithRGBA(1,1,1,1);
		QTest::newRow("opaque black")			<< VuoColor_makeWithRGBA(0,0,0,1);
		QTest::newRow("transparent white")		<< VuoColor_makeWithRGBA(1,1,1,0);
		QTest::newRow("transparent black")		<< VuoColor_makeWithRGBA(0,0,0,0);
		QTest::newRow("semitransparent orange")	<< VuoColor_makeWithRGBA(0.7490196, 0.2901961, 0.1960784, 0.5019608);
	}
	void testImageCopy_verify(VuoImage image, VuoColor color)
	{
		const unsigned char *imageBuffer = VuoImage_getBuffer(image, GL_BGRA);
		// VuoColor is unpremultiplied, yet VuoImage_getBuffer() returns premultiplied colors,
		// so we need to premultiply the VuoColor before comparing.
		// For unknown reasons the green/blue values fluctuate between 127 and 128, so use a tolerance of 2.
		QVERIFY(fabs(imageBuffer[2] - color.a*255*color.r) < 2.);
		QVERIFY(fabs(imageBuffer[1] - color.a*255*color.g) < 2.);
		QVERIFY(fabs(imageBuffer[0] - color.a*255*color.b) < 2.);
		QVERIFY(fabs(imageBuffer[3] - color.a*255        ) < 2.);
	}
	void testImageCopy()
	{
		QFETCH(VuoColor, color);

		VuoImage image = VuoImage_makeColorImage(color, 640, 480);
		QVERIFY(image);
		VuoLocal(image);

		// Ensure the initial image has the expected color.
		testImageCopy_verify(image, color);

		// Ensure an unflipped copy has the expected color.
		{
			VuoImage copiedImage = VuoImage_makeCopy(image, false);
			QVERIFY(copiedImage);
			VuoLocal(copiedImage);

			testImageCopy_verify(copiedImage, color);
		}

		// Ensure a flipped copy has the expected color.
		{
			VuoImage copiedImage = VuoImage_makeCopy(image, true);
			QVERIFY(copiedImage);
			VuoLocal(copiedImage);

			testImageCopy_verify(copiedImage, color);
		}

		// Create a RECT image for subsequent tests.
		VuoImage rectImage = VuoImage_makeGlTextureRectangleCopy(image);
		VuoLocal(rectImage);

		// Ensure the initial RECT image has the expected color.
		testImageCopy_verify(rectImage, color);

		// Ensure an unflipped copy has the expected color.
		{
			VuoImage copiedImage = VuoImage_makeCopy(rectImage, false);
			QVERIFY(copiedImage);
			VuoLocal(copiedImage);

			testImageCopy_verify(copiedImage, color);
		}

		// Ensure a flipped copy has the expected color.
		{
			VuoImage copiedImage = VuoImage_makeCopy(rectImage, true);
			QVERIFY(copiedImage);
			VuoLocal(copiedImage);

			testImageCopy_verify(copiedImage, color);
		}
	}

	void testImageCopyPerformance()
	{
		unsigned long width = 1920;
		unsigned long height = 1080;

		VuoImage sourceImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), width, height);
		QVERIFY(sourceImage);
		VuoRetain(sourceImage);

		// Test that it worked correctly.
		{
			VuoImage copiedImage = VuoImage_makeCopy(sourceImage, false);
			VuoRetain(copiedImage);

			QCOMPARE(copiedImage->pixelsWide, width);
			QCOMPARE(copiedImage->pixelsHigh, height);

			const unsigned char *imageBuffer = VuoImage_getBuffer(copiedImage, GL_BGRA);
			// For unknown reasons the green/blue values fluctuate between 127 and 128, so use a tolerance of 2.
			QVERIFY(abs(imageBuffer[2] - 255) < 2);
			QVERIFY(abs(imageBuffer[1] - 127) < 2);
			QVERIFY(abs(imageBuffer[0] - 127) < 2);
			QVERIFY(abs(imageBuffer[3] - 255) < 2);

			VuoRelease(copiedImage);
		}

		// Test performance.
		QBENCHMARK {
			VuoImage copiedImage = VuoImage_makeCopy(sourceImage, false);
			VuoRetain(copiedImage);
			VuoRelease(copiedImage);
		}

		VuoRelease(sourceImage);
	}

	void testImageCopyRectanglePerformance()
	{
		unsigned long width = 1920;
		unsigned long height = 1080;

		VuoImage sourceImage = VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,0.5,0.5,1), width, height);
		QVERIFY(sourceImage);
		VuoRetain(sourceImage);

		// Test that it worked correctly.
		{
			VuoImage copiedImage = VuoImage_makeGlTextureRectangleCopy(sourceImage);
			VuoRetain(copiedImage);

			QCOMPARE(copiedImage->pixelsWide, width);
			QCOMPARE(copiedImage->pixelsHigh, height);

			const unsigned char *imageBuffer = VuoImage_getBuffer(copiedImage, GL_BGRA);
			// For unknown reasons the green/blue values fluctuate between 127 and 128, so use a tolerance of 2.
			QVERIFY(abs(imageBuffer[2] - 255) < 2);
			QVERIFY(abs(imageBuffer[1] - 127) < 2);
			QVERIFY(abs(imageBuffer[0] - 127) < 2);
			QVERIFY(abs(imageBuffer[3] - 255) < 2);

			VuoRelease(copiedImage);
		}

		// Test performance.
		QBENCHMARK {
			VuoImage copiedImage = VuoImage_makeGlTextureRectangleCopy(sourceImage);
			VuoRetain(copiedImage);
			VuoRelease(copiedImage);
		}

		VuoRelease(sourceImage);
	}

	void testGetBuffer_data()
	{
		QTest::addColumn<int>("requestedFormat");

		// All the formats listed in VuoImage_getBuffer()'s Doxygen.
		QTest::newRow("GL_RGBA")              << GL_RGBA;
		QTest::newRow("GL_BGRA")              << GL_BGRA;
		QTest::newRow("GL_RGBA16I_EXT")       << GL_RGBA16I_EXT;
		QTest::newRow("GL_RGBA16F_ARB")       << GL_RGBA16F_ARB;
		QTest::newRow("GL_RGBA32F_ARB")       << GL_RGBA32F_ARB;
		QTest::newRow("GL_LUMINANCE")         << GL_LUMINANCE;
		QTest::newRow("GL_LUMINANCE_ALPHA")   << GL_LUMINANCE_ALPHA;
		QTest::newRow("GL_DEPTH_COMPONENT16") << GL_DEPTH_COMPONENT16;
	}
	void testGetBuffer()
	{
		QFETCH(int, requestedFormat);


		// Create a color image and a depth image.
		VuoSceneRenderer sr = VuoSceneRenderer_make(1);
		VuoRetain(sr);

		VuoSceneObject rootSceneObject = VuoSceneObject_make(NULL, NULL, VuoTransform_makeIdentity(), VuoListCreate_VuoSceneObject());
		VuoSceneObject_retain(rootSceneObject);

		VuoSceneRenderer_setRootSceneObject(sr, rootSceneObject);
		VuoSceneRenderer_setCameraName(sr, NULL, true);
		VuoSceneRenderer_regenerateProjectionMatrix(sr, 1920, 1080);
		VuoImage colorImage, depthImage;
		VuoSceneRenderer_renderToImage(sr, &colorImage, VuoImageColorDepth_16, VuoMultisample_Off, &depthImage);
		VuoRetain(colorImage);
		VuoRetain(depthImage);


		const unsigned char *pixels = VuoImage_getBuffer(
			requestedFormat == GL_DEPTH_COMPONENT16 ? depthImage : colorImage,
			requestedFormat);
		QVERIFY(pixels);


		VuoRelease(depthImage);
		VuoRelease(colorImage);
		VuoSceneObject_release(rootSceneObject);
		VuoRelease(sr);
	}

	/**
	 * Ensure Vuo can read alpha-transparent BMP files saved by several apps.
	 */
	void testFetchImageBMPAlpha_data()
	{
		QTest::addColumn<bool>("dummy");

		QTest::newRow("bmp-alpha-gimp.bmp")       << true;
		QTest::newRow("bmp-alpha-osxpreview.bmp") << true;
		QTest::newRow("bmp-alpha-photoshop.bmp")  << true;
		QTest::newRow("bmp-alpha-vuo.bmp")        << true;
	}
	void testFetchImageBMPAlpha()
	{
		VuoImage i = VuoImage_get((QString("resources/") + QTest::currentDataTag()).toUtf8().data());
		QVERIFY(i);
		VuoLocal(i);

		QCOMPARE(i->pixelsWide, 2UL);
		QCOMPARE(i->pixelsHigh, 1UL);

		const unsigned char *imageBuffer = VuoImage_getBuffer(i, GL_BGRA);
//		VLog("%d %d %d %d",imageBuffer[2],imageBuffer[1],imageBuffer[0],imageBuffer[3]);
//		VLog("%d %d %d %d",imageBuffer[6],imageBuffer[5],imageBuffer[4],imageBuffer[7]);
		QEXPECT_FAIL("bmp-alpha-gimp.bmp", "https://sourceforge.net/p/freeimage/bugs/266/", Abort);
		QVERIFY(abs(imageBuffer[2] -  84) < 2);
		QVERIFY(abs(imageBuffer[1] -  67) < 2);
		QVERIFY(abs(imageBuffer[0] -  36) < 2);
		QVERIFY(abs(imageBuffer[3] -  84) < 2);
		QVERIFY(abs(imageBuffer[6] - 167) < 2);
		QVERIFY(abs(imageBuffer[5] - 128) < 2);
		QVERIFY(abs(imageBuffer[4] -  60) < 2);
		QVERIFY(abs(imageBuffer[7] - 168) < 2);
	}

	/**
	 * Ensure Vuo reads images into the expected texture types.
	 *
	 * Test images from http://www.schaik.com/pngsuite/
	 * ("Permission to use, copy, modify and distribute these images for any
	 * purpose and without fee is hereby granted. (c) Willem van Schaik, 1996, 2011").
	 */
	void testFetchImageType_data()
	{
		QTest::addColumn<GLint>("glInternalFormat");

		QTest::newRow("basn0g01.png") << GL_LUMINANCE8;       // black & white
		QTest::newRow("basn0g02.png") << GL_LUMINANCE8;       // 2 bit (4 level) grayscale
		QTest::newRow("basn0g04.png") << GL_LUMINANCE8;       // 4 bit (16 level) grayscale
		QTest::newRow("basn0g08.png") << GL_LUMINANCE8;       // 8 bit (256 level) grayscale
		QTest::newRow("basn0g16.png") << GL_LUMINANCE16F_ARB; // 16 bit (64k level) grayscale
		QTest::newRow("basn2c08.png") << GL_RGB;              // 3x8 bits rgb color
		QTest::newRow("basn2c16.png") << GL_RGB16F_ARB;       // 3x16 bits rgb color
		QTest::newRow("basn3p01.png") << GL_RGB;              // 1 bit (2 color) paletted
		QTest::newRow("basn3p02.png") << GL_RGB;              // 2 bit (4 color) paletted
		QTest::newRow("basn3p04.png") << GL_RGB;              // 4 bit (16 color) paletted
		QTest::newRow("basn3p08.png") << GL_RGB;              // 8 bit (256 color) paletted
		QTest::newRow("basn4a08.png") << GL_LUMINANCE_ALPHA;  // 8 bit grayscale + 8 bit alpha-channel
		QTest::newRow("basn4a16.png") << GL_LUMINANCE_ALPHA;  // 16 bit grayscale + 16 bit alpha-channel
		QTest::newRow("basn6a08.png") << GL_RGBA;             // 3x8 bits rgb color + 8 bit alpha-channel
		QTest::newRow("basn6a16.png") << GL_RGBA16F_ARB;      // 3x16 bits rgb color + 16 bit alpha-channel

		QTest::newRow("../gif-alpha.gif") << GL_RGBA;
	}
	void testFetchImageType()
	{
		QFETCH(GLint, glInternalFormat);

		VuoImage i = VuoImage_get((QString("resources/SchaikPngSuite/") + QTest::currentDataTag()).toUtf8().data());
		QVERIFY(i);
		VuoLocal(i);

		QCOMPARE(i->pixelsWide, 32UL);
		QCOMPARE(i->pixelsHigh, 32UL);

		QEXPECT_FAIL("basn0g04.png", "FreeImage unexpectedly reports this greyscale image as RGB", Abort);
		QEXPECT_FAIL("basn4a08.png", "FreeImage unexpectedly reports this greyscale+alpha image as RGBA", Abort);
		QEXPECT_FAIL("basn4a16.png", "FreeImage unexpectedly reports this greyscale+alpha image as RGBA", Abort);
		QVERIFY2(glInternalFormat == i->glInternalFormat, QString("expected %1, got %2")
			.arg(VuoGl_stringForConstant(glInternalFormat))
			.arg(VuoGl_stringForConstant(i->glInternalFormat))
			.toUtf8().constData());
	}

	/**
	 * Ensure Vuo reads image scaleFactor properly.
	 */
	void testFetchImageScaleFactor_data()
	{
		QTest::addColumn<double>("expectedScaleFactor");

		QTest::newRow("spinbox-inc.png")    << 1.;
		QTest::newRow("spinbox-inc@2x.png") << 2.;
		QTest::newRow("spinbox-inc@3x.png") << 3.;
	}
	void testFetchImageScaleFactor()
	{
		QFETCH(double, expectedScaleFactor);

		VuoImage i = VuoImage_get((QString("resources/") + QTest::currentDataTag()).toUtf8().data());
		QVERIFY(i);
		VuoLocal(i);

		QCOMPARE(i->scaleFactor, expectedScaleFactor);
	}

	/**
	 * Ensure Vuo reads huge images without crashing.
	 * https://b33p.net/kosada/node/14063
	 */
	void testFetchHugeImage_data()
	{
		QTest::addColumn<unsigned long>("expectedWidth");
		QTest::addColumn<unsigned long>("expectedHeight");

		QTest::newRow("17000x17000.png") << 17000UL << 17000UL;
	}
	void testFetchHugeImage()
	{
		QFETCH(unsigned long, expectedWidth);
		QFETCH(unsigned long, expectedHeight);

		VuoImage i = VuoImage_get((QString("resources/") + QTest::currentDataTag()).toUtf8().data());
		QEXPECT_FAIL("17000x17000.png", "Vuo doesn't currently support working with images larger than OpenGL's limit (but it still shouldn't crash).  https://b33p.net/kosada/node/7116", Abort);
		QVERIFY(i);
		QCOMPARE(i->pixelsWide, expectedWidth);
		QCOMPARE(i->pixelsHigh, expectedHeight);
		VuoLocal(i);
	}
};

QTEST_APPLESS_MAIN(TestVuoImage)

#include "TestVuoImage.moc"
