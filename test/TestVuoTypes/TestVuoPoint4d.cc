/**
 * @file
 * TestVuoPoint4d implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoPoint4d.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint4d);

/**
 * Tests the VuoPoint4d type.
 */
class TestVuoPoint4d : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoPoint4d>("value");
		QTest::addColumn<bool>("testStringFromValue");

		{
			VuoPoint4d p;
			p.x = -0.5;
			p.y = 0.5;
			p.z = 1;
			p.w = 127;
			QTest::newRow("different values") << "{\"x\":-0.5,\"y\":0.5,\"z\":1.0,\"w\":127.0}" << p << true;
			QTest::newRow("different values array") << QUOTE([-.5,0.5,1,127]) << p << false;
			QTest::newRow("different values text") << QUOTE("-.5,.5, 1,127") << p << false;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint4d, value);
		QFETCH(bool, testStringFromValue);

		VuoPoint4d p = VuoMakeRetainedFromString(initializer.toUtf8().constData(), VuoPoint4d);

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);
		QCOMPARE(p.z,value.z);
		QCOMPARE(p.w,value.w);

		if (testStringFromValue)
			QCOMPARE(VuoPoint4d_getString(p), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint4d)

#include "TestVuoPoint4d.moc"
