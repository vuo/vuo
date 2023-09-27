/**
 * @file
 * TestVuoFont implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoFont.h"
}

/**
 * Tests the VuoFont type.
 */
class TestVuoFont : public QObject
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
										<< "No font 0pt<ul><li>color <span style='background-color:#ffffff;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> 1.00, 1.00, 1.00, 1.00</li><li>Left-aligned</li><li>character spacing: 1</li><li>line spacing: 1</li></ul>";

		QTest::newRow("font")			<< (const char*)VuoFont_getString(VuoFont_make(
																					"Helvetica",
																					12,
																					true,
																					VuoColor_makeWithRGBA(1,1,1,1),
																					VuoHorizontalAlignment_Right,
																					0,
																					2))
										<< true
										<< "Helvetica 12pt<ul><li>color <span style='background-color:#ffffff;'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span> 1.00, 1.00, 1.00, 1.00</li><li>underlined</li><li>Right-aligned</li><li>character spacing: 0</li><li>line spacing: 2</li></ul>";
	}
	void testSerializationAndSummary()
	{
		QFETCH(QString, value);
		QFETCH(bool, testReverse);
		QFETCH(QString, summary);

		VuoFont f = VuoMakeRetainedFromString(value.toUtf8().data(), VuoFont);
		if (testReverse)
			QCOMPARE(QString::fromUtf8(VuoFont_getString(f)), value);
		QCOMPARE(QString::fromUtf8(VuoFont_getSummary(f)), summary);
		VuoFont_release(f);
	}
};

QTEST_APPLESS_MAIN(TestVuoFont)

#include "TestVuoFont.moc"
