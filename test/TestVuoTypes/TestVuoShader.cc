/**
 * @file
 * TestVuoShader implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoImageGet.h"
#include "VuoShader.h"
}

Q_DECLARE_METATYPE(VuoShader);

/**
 * Tests the VuoShader type.
 */
class TestVuoShader : public QObject
{
	Q_OBJECT

private slots:

	void testNull()
	{
		QCOMPARE(QString::fromUtf8(VuoShader_getString(NULL)), QString("null"));
		QCOMPARE(QString::fromUtf8(VuoShader_getSummary(NULL)), QString("No shader"));
	}

	void testSerialization_data()
	{
		QTest::addColumn<QString>("valueAsString");
		QTest::addColumn<QString>("summary");

		{
			QTest::newRow("empty string") << "" << "";
		}
		{
			VuoShader shader = VuoShader_makeDefaultShader();
			char *valueAsString = VuoShader_getString(shader);
			QTest::newRow("VuoShader") << QString(valueAsString) << QString(shader->name);
			free(valueAsString);
		}
	}
	void testSerialization()
	{
		QFETCH(QString, valueAsString);
		QFETCH(QString, summary);

		VuoShader value = VuoMakeRetainedFromString(valueAsString.toUtf8().constData(), VuoShader);
		if (value)
			QCOMPARE(QString(value->name), summary);
		else
			QCOMPARE(QString(""), summary);
		VuoRelease(value);
	}

	void testMakeDefaultShaderPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();

		QBENCHMARK {
			VuoShader s = VuoShader_makeDefaultShader();
			VuoRetain(s);
			QVERIFY(VuoShader_getAttributeLocations(s, VuoMesh_IndividualTriangles, glContext, NULL, NULL, NULL, NULL));
			VuoRelease(s);
		}

		VuoGlContext_disuse(glContext);
	}

	void testMakeImageShaderPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();

		QBENCHMARK {
			VuoShader s = VuoShader_makeUnlitImageShader(NULL, 1);
			VuoRetain(s);
			QVERIFY(VuoShader_getAttributeLocations(s, VuoMesh_IndividualTriangles, glContext, NULL, NULL, NULL, NULL));
			VuoRelease(s);
		}

		VuoGlContext_disuse(glContext);
	}

	void testGetUniformLocationPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();

		VuoShader s = VuoShader_makeLitColorShader(VuoColor_makeWithRGBA(1,1,1,1), VuoColor_makeWithRGBA(1,1,1,1), 1);
		VuoRetain(s);

		VuoGlProgram program;
		QVERIFY(VuoShader_activate(s, VuoMesh_IndividualTriangles, glContext, &program));

		// A random selection of uniforms
		vector<string> uniformNames;
		uniformNames.push_back("specularPower");
		uniformNames.push_back("diffuseColor");
		uniformNames.push_back("specularColor");
		uniformNames.push_back("cameraPosition");
		uniformNames.push_back("useFisheyeProjection");
		uniformNames.push_back("ambientBrightness");
		uniformNames.push_back("pointLights[15].sharpness");
		uniformNames.push_back("spotLightCount");
		uniformNames.push_back("projectionMatrix");
		uniformNames.push_back("spotLights[1].color");
		int uniformNameCount = uniformNames.size();
		int i = 0;

		QBENCHMARK {
			QVERIFY(VuoGlProgram_getUniformLocation(program, uniformNames[i++ % uniformNameCount].c_str()) != -1);
		}

		VuoShader_deactivate(s, VuoMesh_IndividualTriangles, glContext);
		VuoRelease(s);
		VuoGlContext_disuse(glContext);
	}

	void testOpaque_data()
	{
		QTest::addColumn<VuoShader>("shader");
		QTest::addColumn<bool>("expectedOpacity");

		QTest::newRow("default") << VuoShader_makeDefaultShader() << true;

		QTest::newRow("color opaque")      << VuoShader_makeUnlitColorShader((VuoColor){1,1,1,1 }) << true;
		QTest::newRow("color transparent") << VuoShader_makeUnlitColorShader((VuoColor){1,1,1,.5}) << false;

		VuoText testPNGFilename = VuoText_make("resources/SchaikPngSuite/basn2c08.png");
		VuoLocal(testPNGFilename);
		QTest::newRow("image NULL opaque")      << VuoShader_makeUnlitImageShader(NULL,                                                  1 ) << true;  // It's not going to render anything, so no need to apply the expensive potentially-transparent treatment.
		QTest::newRow("image PNG opaque")       << VuoShader_makeUnlitImageShader(VuoImage_get(testPNGFilename),                         1 ) << true;
		QTest::newRow("image RGB opaque")       << VuoShader_makeUnlitImageShader(VuoImage_makeColorImage((VuoColor){1,1,1,1 }, 10, 10), 1 ) << true;
		QTest::newRow("image RGB transparent")  << VuoShader_makeUnlitImageShader(VuoImage_makeColorImage((VuoColor){1,1,1,1 }, 10, 10), .5) << false;
		QTest::newRow("image RGBA opaque")      << VuoShader_makeUnlitImageShader(VuoImage_makeColorImage((VuoColor){1,1,1,.5}, 10, 10), 1 ) << false;

		{
			VuoShader s = VuoShader_makeLinearGradientShader();
			const char *colorsStr = VUO_STRINGIFY(["#445566"]);
			VuoList_VuoColor colors = VuoMakeRetainedFromString(colorsStr, VuoList_VuoColor);
			VuoShader_setLinearGradientShaderValues(s, colors, (VuoPoint2d){0,0}, (VuoPoint2d){1,1}, 1, 0);
			VuoRelease(colors);
			QTest::newRow("gradient opaque") << s << true;
		}

		{
			VuoShader s = VuoShader_makeLinearGradientShader();
			const char *colorsStr = VUO_STRINGIFY(["#44556680"]);
			VuoList_VuoColor colors = VuoMakeRetainedFromString(colorsStr, VuoList_VuoColor);
			VuoShader_setLinearGradientShaderValues(s, colors, (VuoPoint2d){0,0}, (VuoPoint2d){1,1}, 1, 0);
			VuoRelease(colors);
			QTest::newRow("gradient transparent") << s << false;
		}
	}
	void testOpaque()
	{
		QFETCH(VuoShader, shader);
		QFETCH(bool, expectedOpacity);

		QCOMPARE(VuoShader_isOpaque(shader), expectedOpacity);
	}
};

QTEST_APPLESS_MAIN(TestVuoShader)

#include "TestVuoShader.moc"
