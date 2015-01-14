/**
 * @file
 * TestVuoImage implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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

	void testMakeFromBufferPerformance()
	{
		unsigned int width = 1920;
		unsigned int height = 1080;
		unsigned char *buffer = (unsigned char *)malloc(width*height*4);

		QBENCHMARK {
			VuoImage i = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height);
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

		VuoShader s = VuoShader_make("solid color shader", solidColorVertexShaderSource, solidColorFragmentShaderSource);
		VuoRetain(s);
		QBENCHMARK {
			VuoImage i = VuoImageRenderer_draw(ir, s, width, height);
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
		VuoImage sourceImage = VuoImage_makeFromBuffer(buffer, GL_RGBA, width, height);
		free(buffer);
		VuoRetain(sourceImage);

		VuoGlContext glContext = VuoGlContext_use();

		VuoImageRenderer ir = VuoImageRenderer_make(glContext);
		VuoRetain(ir);

		VuoShader s = VuoShader_make("ripple shader", VuoShader_getDefaultVertexShader(), rippleFragmentShaderSource);
		VuoRetain(s);

		// Associate the input image with the shader.
		VuoShader_resetTextures(s);
		VuoShader_addTexture(s, glContext, "texture", sourceImage);

		// Feed parameters to the shader.
		VuoShader_setUniformFloat(s, glContext, "angle", 135.*M_PI/180.);
		VuoShader_setUniformFloat(s, glContext, "amplitude", .1);
		VuoShader_setUniformFloat(s, glContext, "wavelength", 0.05*M_PI*2.);
		VuoShader_setUniformFloat(s, glContext, "phase", 0.*M_PI*2.);

		QBENCHMARK {
			VuoImage i = VuoImageRenderer_draw(ir, s, width, height);
			VuoRetain(i);
			VuoRelease(i);
		}
		VuoRelease(s);
		VuoRelease(ir);
		VuoGlContext_disuse(glContext);
		VuoRelease(sourceImage);
	}

};

QTEST_APPLESS_MAIN(TestVuoImage)

#include "TestVuoImage.moc"
