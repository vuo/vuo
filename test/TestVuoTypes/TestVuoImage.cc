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
}

/**
 * Tests the VuoImage type.
 */
class TestVuoImage : public QObject
{
	Q_OBJECT

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

		QTest::newRow("texture")		<< "{\"glTextureName\":42,\"pixelsWide\":640,\"pixelsHigh\":480}"
										<< true
										<< "GL Texture (ID 42)<br>640x480";

		QTest::newRow("make")			<< QString(VuoImage_stringFromValue(VuoImage_make(42,640,480)))
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
};

QTEST_APPLESS_MAIN(TestVuoImage)

#include "TestVuoImage.moc"
