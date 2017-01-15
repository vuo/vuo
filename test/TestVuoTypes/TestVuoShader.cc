/**
 * @file
 * TestVuoShader implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoShader.h"
}

/**
 * Tests the VuoShader type.
 */
class TestVuoShader : public QObject
{
	Q_OBJECT

private slots:

	void testNull()
	{
		QCOMPARE(QString::fromUtf8(VuoShader_getString(NULL)), QString("0"));
		QCOMPARE(QString::fromUtf8(VuoShader_getSummary(NULL)), QString("(no shader)"));
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

		VuoShader value = VuoShader_makeFromString(valueAsString.toUtf8().data());
		if (value)
			QCOMPARE(QString(value->name), summary);
		else
			QCOMPARE(QString(""), summary);
	}

	void testMakeDefaultShaderPerformance()
	{
		VuoGlContext glContext = VuoGlContext_use();

		QBENCHMARK {
			VuoShader s = VuoShader_makeDefaultShader();
			VuoRetain(s);
			VuoShader_getAttributeLocations(s, VuoMesh_IndividualTriangles, glContext, NULL, NULL, NULL, NULL, NULL);
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
			VuoShader_getAttributeLocations(s, VuoMesh_IndividualTriangles, glContext, NULL, NULL, NULL, NULL, NULL);
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
		VuoShader_cleanupContext(glContext);
		VuoRelease(s);
		VuoGlContext_disuse(glContext);
	}
};

QTEST_APPLESS_MAIN(TestVuoShader)

#include "TestVuoShader.moc"
