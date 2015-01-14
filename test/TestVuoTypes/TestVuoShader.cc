/**
 * @file
 * TestVuoShader implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
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
			QTest::newRow("empty string") << "" << "default checkerboard shader";
		}
		{
			VuoShader shader = VuoShader_valueFromString("");
			shader->summary = strdup("test summary");
			char *valueAsString = VuoShader_stringFromValue(shader);
			QTest::newRow("VuoShader") << QString(valueAsString) << QString(shader->summary);
			free(valueAsString);
		}
	}
	void testSerialization()
	{
		QFETCH(QString, valueAsString);
		QFETCH(QString, summary);

		VuoShader value = VuoShader_valueFromString(valueAsString.toUtf8().data());
		QVERIFY(value != NULL);
		QCOMPARE(QString(value->summary), summary);
	}

	void testMakeDefaultShaderPerformance()
	{
		QBENCHMARK {
			VuoShader s = VuoShader_valueFromString("");
			VuoRetain(s);
			VuoRelease(s);
		}
	}

	void testMakeImageShaderPerformance()
	{
		QBENCHMARK {
			VuoShader s = VuoShader_makeImageShader();
			VuoRetain(s);
			VuoRelease(s);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoShader)

#include "TestVuoShader.moc"
