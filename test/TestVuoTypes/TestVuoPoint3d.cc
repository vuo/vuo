/**
 * @file
 * TestVuoPoint3d implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoPoint3d.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoPoint3d);

/**
 * Tests the VuoPoint3d type.
 */
class TestVuoPoint3d : public QObject
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
		QTest::addColumn<VuoPoint3d>("value");

		{
			VuoPoint3d p;
			p.x = -0.999;
			p.y = 0.42;
			p.z = 0.22;
			QTest::newRow("different values") << "{\"x\":-0.999000,\"y\":0.420000,\"z\":0.220000}" << p;
		}
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoPoint3d, value);

		VuoPoint3d p = VuoPoint3d_valueFromString(initializer.toUtf8().constData());

		QCOMPARE(p.x,value.x);
		QCOMPARE(p.y,value.y);
		QCOMPARE(p.z,value.z);

		QCOMPARE(VuoPoint3d_stringFromValue(p), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoPoint3d)

#include "TestVuoPoint3d.moc"

