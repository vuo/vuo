/**
 * @file
 * TestVuoImage implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoImage.h"
#include "VuoGlContext.h"
#include "VuoShader.h"
#include "VuoImageRenderer.h"
}

#include <OpenGL/CGLMacro.h>

// Be able to use these types in QTest::addColumn()
Q_DECLARE_METATYPE(VuoImage);

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
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testNull()
	{
		QCOMPARE(QString::fromUtf8(VuoImage_stringFromValue(NULL)), QString("{}"));
		QCOMPARE(QString::fromUtf8(VuoImage_interprocessStringFromValue(NULL)), QString("{}"));
		QCOMPARE(QString::fromUtf8(VuoImage_summaryFromValue(NULL)), QString("(no image)"));
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

		QTest::newRow("make")			<< QString(VuoImage_stringFromValue(VuoImage_make(42,0,640,480)))
										<< true
										<< "GL Texture (ID 42)<br>640x480";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, valid);
		QFETCH(QString, summary);

		VuoImage t = VuoImage_valueFromString(value.toUtf8().data());
		if (valid)
		{
			QCOMPARE(QString::fromUtf8(VuoImage_stringFromValue(t)), value);
			QCOMPARE(QString::fromUtf8(VuoImage_summaryFromValue(t)), summary);
		}
	}

	void testJsonColorImage()
	{
		VuoImage image = VuoImage_valueFromString(QUOTE({"color":{"r":0.5,"g":1,"b":0,"a":1},"pixelsWide":640,"pixelsHigh":480}));
		QVERIFY(image);

		VuoRetain(image);
		unsigned char *imageBuffer = VuoImage_copyBuffer(image, GL_RGBA);
		QVERIFY(abs(imageBuffer[0] - 127) < 2);
		QVERIFY(abs(imageBuffer[1] - 255) < 2);
		QVERIFY(abs(imageBuffer[2] -   0) < 2);
		QVERIFY(abs(imageBuffer[3] - 255) < 2);
		VuoRelease(image);
	}

	void testImageEquality_data()
	{
		QTest::addColumn<VuoImage>("a");
		QTest::addColumn<VuoImage>("b");
		QTest::addColumn<bool>("expectedEquality");

		QTest::newRow("null null")			<< (VuoImage)NULL
											<< (VuoImage)NULL
											<< true;

		QTest::newRow("null nonnull")		<< (VuoImage)NULL
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< false;

		QTest::newRow("nonnull null")		<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< (VuoImage)NULL
											<< false;

		QTest::newRow("same color")			<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< true;

		QTest::newRow("different color")	<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,1),1,1)
											<< VuoImage_makeColorImage(VuoColor_makeWithRGBA(1,1,1,.1),1,1)
											<< false;
	}
	void testImageEquality()
	{
		QFETCH(VuoImage, a);
		QFETCH(VuoImage, b);
		QFETCH(bool, expectedEquality);

		bool actualEquality = VuoImage_areEqual(a,b);
		QCOMPARE(actualEquality, expectedEquality);
	}

	void testMakeFromBufferPerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);

		QBENCHMARK {
			VuoImage i = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height, VuoImageColorDepth_8);
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
		VuoImage localImage = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height);
		free(buffer);
		VuoRetain(localImage);

		QBENCHMARK {
			json_object *interprocessImageJson = VuoImage_interprocessJsonFromValue(localImage);
			VuoImage remoteImage = VuoImage_valueFromJson(interprocessImageJson);
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
		unsigned char *imageBuffer = VuoImage_copyBuffer(i, GL_RGBA);
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
		unsigned char *imageBuffer = VuoImage_copyBuffer(i, GL_RGBA);
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
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height, VuoImageColorDepth_8);
		free(buffer);
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
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height, VuoImageColorDepth_8);
		free(buffer);
		VuoRetain(sourceImage);

		QBENCHMARK {
			VuoImage i = VuoImage_blur(sourceImage, 1, FALSE);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(sourceImage);
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
			VuoImage copiedImage = VuoImage_makeCopy(sourceImage);
			VuoRetain(copiedImage);

			QCOMPARE(copiedImage->pixelsWide, width);
			QCOMPARE(copiedImage->pixelsHigh, height);

			unsigned char *imageBuffer = VuoImage_copyBuffer(copiedImage, GL_RGBA);
			// For unknown reasons the green/blue values fluctuate between 127 and 128, so use a tolerance of 2.
			QVERIFY(abs(imageBuffer[0] - 255) < 2);
			QVERIFY(abs(imageBuffer[1] - 127) < 2);
			QVERIFY(abs(imageBuffer[2] - 127) < 2);
			QVERIFY(abs(imageBuffer[3] - 255) < 2);
			free(imageBuffer);

			VuoRelease(copiedImage);
		}

		// Test performance.
		QBENCHMARK {
			VuoImage copiedImage = VuoImage_makeCopy(sourceImage);
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

			unsigned char *imageBuffer = VuoImage_copyBuffer(copiedImage, GL_RGBA);
			// For unknown reasons the green/blue values fluctuate between 127 and 128, so use a tolerance of 2.
			QVERIFY(abs(imageBuffer[0] - 255) < 2);
			QVERIFY(abs(imageBuffer[1] - 127) < 2);
			QVERIFY(abs(imageBuffer[2] - 127) < 2);
			QVERIFY(abs(imageBuffer[3] - 255) < 2);
			free(imageBuffer);

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
};

QTEST_APPLESS_MAIN(TestVuoImage)

#include "TestVuoImage.moc"
