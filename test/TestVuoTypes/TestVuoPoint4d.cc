/**
 * @file
 * TestVuoPoint4d implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
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
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoPoint4d>("value");

		{
			VuoPoint4d p;
			p.x = -0.999;
			p.y = 0.42;
			p.z = 0.22;
			p.w = 127;
			QTest::newRow("different values") << "{\"x\":-0.999000,\"y\":0.420000,\"z\":0.220000,\"w\":127.000000}" << p;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint4d, value);

		VuoPoint4d p = VuoPoint4d_makeFromString(initializer.toUtf8().constData());

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);
		QCOMPARE(p.z,value.z);
		QCOMPARE(p.w,value.w);

		QCOMPARE(VuoPoint4d_getString(p), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint4d)

#include "TestVuoPoint4d.moc"

