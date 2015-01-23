/**
 * @file
 * TestVuoInteger implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoInteger.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoInteger);

/**
 * Tests the VuoInteger type.
 */
class TestVuoInteger : public QObject
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
		QTest::addColumn<VuoInteger>("value");
		QTest::addColumn<bool>("testTypeValueAsString");

		QTest::newRow("zero") << "0" << (VuoInteger)0 << true;
		QTest::newRow("positive") << "1" << (VuoInteger)1 << true;
		QTest::newRow("negative") << "-1" << (VuoInteger)-1 << true;
		QTest::newRow("positive max") << "9223372036854775807" << (VuoInteger)9223372036854775807 << true;
		// http://lists.cs.uiuc.edu/pipermail/cfe-dev/2012-February/019751.html
		QTest::newRow("negative min") << "-9223372036854775808" << (VuoInteger)-9223372036854775807-1 << true;
		QTest::newRow("noninteger") << "Otto von Bismarck" << (VuoInteger)0 << false;
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoInteger, value);
		QFETCH(bool, testTypeValueAsString);

		QCOMPARE(VuoInteger_valueFromString(initializer.toUtf8().constData()), value);
		if (testTypeValueAsString)
			QCOMPARE(VuoInteger_stringFromValue(value), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoInteger)

#include "TestVuoInteger.moc"
