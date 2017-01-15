/**
 * @file
 * TestVuoImage implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoImage.h"
#include "VuoGlContext.h"
#include "VuoShader.h"
#include "VuoImageGet.h"
#include "VuoImageRenderer.h"
#include "VuoImageBlur.h"
#include "VuoSceneRenderer.h"
}

#include <OpenGL/CGLMacro.h>

// Be able to use these types in QTest::addColumn()
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

		QTest::newRow("texture")		<< "{\"glTextureName\":42,\"glInternalFormat\":0,\"pixelsWide\":640,\"pixelsHigh\":480}"
										<< true
										<< "GL Texture (ID 42)<br>640x480";

		QTest::newRow("make")			<< QString(VuoImage_getString(VuoImage_make(43,0,640,480)))
										<< true
										<< "GL Texture (ID 43)<br>640x480";
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
		VuoGlContext glContext = VuoGlContext_use();

		VuoImageRenderer ir = VuoImageRenderer_make(glContext);
		VuoRetain(ir);

		VuoShader s = VuoShader_make("Solid Color Shader");
		VuoShader_addSource(s, VuoMesh_IndividualTriangles, solidColorVertexShaderSource, NULL, solidColorFragmentShaderSource);
		VuoRetain(s);

		VuoImage i = VuoImageRenderer_draw(ir, s, width, height, VuoImageColorDepth_8);
		VuoRetain(i);
		const unsigned char *imageBuffer = VuoImage_getBuffer(i, GL_BGRA);
		QCOMPARE(imageBuffer[0], (unsigned char)255);
		QCOMPARE(imageBuffer[1], (unsigned char)255);
		QCOMPARE(imageBuffer[2], (unsigned char)255);
		QCOMPARE(imageBuffer[3], (unsigned char)255);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_draw(ir, s, width, height, VuoImageColorDepth_8);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
		VuoRelease(ir);
		VuoGlContext_disuse(glContext);
	}

	void testImageRendererPerformance16bpc()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		VuoGlContext glContext = VuoGlContext_use();

		VuoImageRenderer ir = VuoImageRenderer_make(glContext);
		VuoRetain(ir);

		VuoShader s = VuoShader_make("Solid Color Shader");
		VuoShader_addSource(s, VuoMesh_IndividualTriangles, solidColorVertexShaderSource, NULL, solidColorFragmentShaderSource);
		VuoRetain(s);

		VuoImage i = VuoImageRenderer_draw(ir, s, width, height, VuoImageColorDepth_16);
		VuoRetain(i);
		const unsigned char *imageBuffer = VuoImage_getBuffer(i, GL_BGRA);
		QCOMPARE(imageBuffer[0], (unsigned char)255);
		QCOMPARE(imageBuffer[1], (unsigned char)255);
		QCOMPARE(imageBuffer[2], (unsigned char)255);
		QCOMPARE(imageBuffer[3], (unsigned char)255);
		VuoRelease(i);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_draw(ir, s, width, height, VuoImageColorDepth_16);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
		VuoRelease(ir);
		VuoGlContext_disuse(glContext);
	}

	void testImageRendererRipplePerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_BGRA, width, height, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });
		VuoRetain(sourceImage);

		VuoGlContext glContext = VuoGlContext_use();

		VuoImageRenderer ir = VuoImageRenderer_make(glContext);
		VuoRetain(ir);

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
			VuoImage i = VuoImageRenderer_draw(ir, s, width, height, VuoImageColorDepth_8);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
		VuoRelease(ir);
		VuoGlContext_disuse(glContext);
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
			VuoImage i = VuoImageBlur_blur(ib, sourceImage, 1, FALSE);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(sourceImage);
		VuoRelease(ib);
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
		VuoGlContext glContext = VuoGlContext_use();
		VuoSceneRenderer sr = VuoSceneRenderer_make(glContext, 1);
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
		VuoGlContext_disuse(glContext);
	}
};

QTEST_APPLESS_MAIN(TestVuoImage)

#include "TestVuoImage.moc"
