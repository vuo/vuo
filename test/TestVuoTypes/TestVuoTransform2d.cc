/**
 * @file
 * TestVuoTransform2d implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoTransform2d.h"
}

/**
 * Tests the VuoTransform2d type.
 */
class TestVuoTransform2d : public QObject
{
	Q_OBJECT

private slots:

	void testSerializationAndSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<bool>("testReverse");
		QTest::addColumn<QString>("summary");

		QTest::newRow("emptystring")	<< ""
										<< false
										<< "identity transform (no change)";

		QTest::newRow("identity")		<< "\"identity\""
										<< true
										<< "identity transform (no change)";

		QTest::newRow("transform")		<< (const char*)VuoTransform2d_getString(VuoTransform2d_make(VuoPoint2d_make(1,1),M_PI/2.,VuoPoint2d_make(2,2)))
										<< true
										<< "<div>translation (1, 1)</div>\n<div>rotation 90°</div>\n<div>scale (2, 2)</div>";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, testReverse);
		QFETCH(QString, summary);

		VuoTransform2d t = VuoMakeRetainedFromString(value.toUtf8().constData(), VuoTransform2d);
		if (testReverse)
			QCOMPARE(QString::fromUtf8(VuoTransform2d_getString(t)), value);
		QCOMPARE(QString::fromUtf8(VuoTransform2d_getSummary(t)), summary);
	}
};

QTEST_APPLESS_MAIN(TestVuoTransform2d)

#include "TestVuoTransform2d.moc"
