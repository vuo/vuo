/**
 * @file
 * TestVuoShader implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
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
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testNull()
	{
		QCOMPARE(QString::fromUtf8(VuoShader_stringFromValue(NULL)), QString("0"));
		QCOMPARE(QString::fromUtf8(VuoShader_summaryFromValue(NULL)), QString("(no shader)"));
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
			char *valueAsString = VuoShader_stringFromValue(shader);
			QTest::newRow("VuoShader") << QString(valueAsString) << QString(shader->name);
			free(valueAsString);
		}
	}
	void testSerialization()
	{
		QFETCH(QString, valueAsString);
		QFETCH(QString, summary);

		VuoShader value = VuoShader_valueFromString(valueAsString.toUtf8().data());
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
};

QTEST_APPLESS_MAIN(TestVuoShader)

#include "TestVuoShader.moc"
