/**
 * @file
 * TestVuoInteger implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

		QCOMPARE(VuoInteger_makeFromString(initializer.toUtf8().constData()), value);
		if (testTypeValueAsString)
			QCOMPARE(VuoInteger_getString(value), initializer.toUtf8().constData());
	}

	void testRandom_data()
	{
		QTest::addColumn<VuoInteger>("minimum");
		QTest::addColumn<VuoInteger>("maximum");
		QTest::addColumn<VuoInteger>("expectedMinimumUniqueValueCount");

		QTest::newRow("zero")			<< (VuoInteger)   0 << (VuoInteger)  0 << (VuoInteger)1;
		QTest::newRow("positive range")	<< (VuoInteger)   1 << (VuoInteger)100 << (VuoInteger)5;	// Conservative, to keep the test from sporadically failing when we're extraordinarily lucky.
		QTest::newRow("negative range")	<< (VuoInteger)-100 << (VuoInteger) -1 << (VuoInteger)5;
	}
	void testRandom()
	{
		QFETCH(VuoInteger, minimum);
		QFETCH(VuoInteger, maximum);
		QFETCH(VuoInteger, expectedMinimumUniqueValueCount);

		unsigned short state[3];
		VuoInteger_setRandomState(state, 0);

		QSet<VuoInteger> generated;
		QSet<VuoInteger> generatedWithState;
		for (int i = 0; i < 1000; ++i)
		{
			{
				VuoInteger v = VuoInteger_random(minimum, maximum);
				QVERIFY(v >= minimum);
				QVERIFY(v <= maximum);
				generated.insert(v);
			}
			{
				VuoInteger v = VuoInteger_randomWithState(state, minimum, maximum);
				QVERIFY(v >= minimum);
				QVERIFY(v <= maximum);
				generatedWithState.insert(v);
			}
		}
		QVERIFY(generated.size() >= expectedMinimumUniqueValueCount);
		QVERIFY(generatedWithState.size() >= expectedMinimumUniqueValueCount);
	}

	void testWrap_data()
	{
		QTest::addColumn<VuoInteger>("value");
		QTest::addColumn<VuoInteger>("minimum");
		QTest::addColumn<VuoInteger>("maximum");
		QTest::addColumn<VuoInteger>("expectedWrappedValue");

		QTest::newRow("0 inside null range")				<< (VuoInteger)0 << (VuoInteger)0 << (VuoInteger)0 << (VuoInteger)0;
		QTest::newRow("1 inside null range")				<< (VuoInteger)1 << (VuoInteger)1 << (VuoInteger)1 << (VuoInteger)1;
		QTest::newRow("0 outside null range")				<< (VuoInteger)0 << (VuoInteger)1 << (VuoInteger)1 << (VuoInteger)1;
		QTest::newRow("1 outside null range")				<< (VuoInteger)1 << (VuoInteger)0 << (VuoInteger)0 << (VuoInteger)0;
		QTest::newRow("0 inside nonnull range")				<< (VuoInteger)0 << (VuoInteger)0 << (VuoInteger)1 << (VuoInteger)0;
		QTest::newRow("1 inside nonnull range")				<< (VuoInteger)1 << (VuoInteger)0 << (VuoInteger)1 << (VuoInteger)1;
		QTest::newRow("0 outside nonnull range")			<< (VuoInteger)0 << (VuoInteger)1 << (VuoInteger)2 << (VuoInteger)2;
		QTest::newRow("1 outside nonnull range")			<< (VuoInteger)1 << (VuoInteger)2 << (VuoInteger)3 << (VuoInteger)3;
		QTest::newRow("0 inside reversed nonnull range")	<< (VuoInteger)0 << (VuoInteger)1 << (VuoInteger)0 << (VuoInteger)0;
		QTest::newRow("1 inside reversed nonnull range")	<< (VuoInteger)1 << (VuoInteger)1 << (VuoInteger)0 << (VuoInteger)1;
	}
	void testWrap()
	{
		QFETCH(VuoInteger, value);
		QFETCH(VuoInteger, minimum);
		QFETCH(VuoInteger, maximum);
		QFETCH(VuoInteger, expectedWrappedValue);

		VuoInteger actualWrappedValue = VuoInteger_wrap(value, minimum, maximum);
		QCOMPARE(actualWrappedValue, expectedWrappedValue);
	}
};

QTEST_APPLESS_MAIN(TestVuoInteger)

#include "TestVuoInteger.moc"
