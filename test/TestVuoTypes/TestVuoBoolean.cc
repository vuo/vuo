/**
 * @file
 * TestVuoBoolean implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoBoolean.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoBoolean);

/**
 * Tests the VuoBoolean type.
 */
class TestVuoBoolean : public QObject
{
	Q_OBJECT

private slots:

	void testStringConversion_data()
	{
		QTest::addColumn<QString>("initializer");
		QTest::addColumn<VuoBoolean>("value");
		QTest::addColumn<bool>("testTypeValueAsString");

		QTest::newRow("false")       << "false"     << (VuoBoolean)false << true;
		QTest::newRow("FALSE")       << "FALSE"     << (VuoBoolean)false << false;

		QTest::newRow("true")        << "true"      << (VuoBoolean)true  << true;
		QTest::newRow("TRUE")        << "TRUE"      << (VuoBoolean)true  << false;

		QTest::newRow("0")           << "0"         << (VuoBoolean)false << false;
		QTest::newRow("1")           << "1"         << (VuoBoolean)true  << false;
		QTest::newRow("2")           << "2"         << (VuoBoolean)true  << false;
		QTest::newRow("-1")          << "-1"        << (VuoBoolean)true  << false;

		QTest::newRow("text: empty") << "\"\""      << (VuoBoolean)false << false;
		QTest::newRow("text: false") << "\"false\"" << (VuoBoolean)false << false;
		QTest::newRow("text: FALSE") << "\"FALSE\"" << (VuoBoolean)false << false;
		QTest::newRow("text: f")     << "\"f\""     << (VuoBoolean)false << false;
		QTest::newRow("text: F")     << "\"F\""     << (VuoBoolean)false << false;
		QTest::newRow("text: no")    << "\"no\""    << (VuoBoolean)false << false;
		QTest::newRow("text: NO")    << "\"NO\""    << (VuoBoolean)false << false;
		QTest::newRow("text: n")     << "\"n\""     << (VuoBoolean)false << false;
		QTest::newRow("text: N")     << "\"N\""     << (VuoBoolean)false << false;

		QTest::newRow("text: true")  << "\"true\""  << (VuoBoolean)true  << false;
		QTest::newRow("text: TRUE")  << "\"TRUE\""  << (VuoBoolean)true  << false;
		QTest::newRow("text: t")     << "\"t\""     << (VuoBoolean)true  << false;
		QTest::newRow("text: T")     << "\"T\""     << (VuoBoolean)true  << false;
		QTest::newRow("text: yes")   << "\"yes\""   << (VuoBoolean)true  << false;
		QTest::newRow("text: YES")   << "\"YES\""   << (VuoBoolean)true  << false;
		QTest::newRow("text: y")     << "\"y\""     << (VuoBoolean)true  << false;
		QTest::newRow("text: Y")     << "\"Y\""     << (VuoBoolean)true  << false;
	}
	void testStringConversion()
	{
		QFETCH(QString, initializer);
		QFETCH(VuoBoolean, value);
		QFETCH(bool, testTypeValueAsString);

		QCOMPARE(VuoBoolean_makeFromString(initializer.toUtf8().constData()), value);
		if (testTypeValueAsString)
			QCOMPARE(VuoBoolean_getString(value), initializer.toUtf8().constData());
	}
};

QTEST_APPLESS_MAIN(TestVuoBoolean)

#include "TestVuoBoolean.moc"
