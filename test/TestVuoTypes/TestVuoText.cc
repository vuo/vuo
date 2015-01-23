/**
 * @file
 * TestVuoText implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoText.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoText);

/**
 * Tests the VuoText type.
 */
class TestVuoText : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

	void testNull()
	{
		// Test NULL strings separately, since you can't pass a NULL QString as data into the other test methods.

		QCOMPARE(QString::fromUtf8(VuoText_make(NULL)), QString(""));

		QCOMPARE(QString::fromUtf8(VuoText_stringFromValue(NULL)), QString("\"\""));

		QCOMPARE(QString::fromUtf8(VuoText_summaryFromValue(NULL)), QString(""));

		QCOMPARE(VuoText_length(NULL), (size_t)0);

		QCOMPARE(QString::fromUtf8(VuoText_substring(NULL, 0, 0)), QString(""));
		QCOMPARE(QString::fromUtf8(VuoText_substring(NULL, 0, 1)), QString(""));

		{
			VuoText textsArray[1] = {
				NULL
			};
			QCOMPARE(QString::fromUtf8(VuoText_append(textsArray, 1)), QString(""));
		}

		{
			VuoText textsArray[3] = {
				"first",
				NULL,
				"third"
			};
			QCOMPARE(QString::fromUtf8(VuoText_append(textsArray, 3)), QString("firstthird"));
		}
	}

	void testSummary_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<QString>("summary");

		QTest::newRow("empty string")		<< ""									<< "";
		QTest::newRow("short")				<< "a"									<< "a";
		QTest::newRow("borderline")			<< "01234567890123456789012345678901"	<< "01234567890123456789012345678901";
		QTest::newRow("long")				<< "012345678901234567890123456789012"	<< "01234567890123456789012345678901...";
		QTest::newRow("UTF8 short")			<< QString::fromUtf8("流")				<< QString::fromUtf8("流");
		QTest::newRow("UTF8 borderline")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛");
		QTest::newRow("UTF8 long")		<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛...");
	}
	void testSummary()
	{
		QFETCH(QString, value);
		QFETCH(QString, summary);

		QEXPECT_FAIL("UTF8 borderline", "@todo: https://b33p.net/kosada/node/3730", Continue);
		QEXPECT_FAIL("UTF8 long", "@todo: https://b33p.net/kosada/node/3730", Continue);
		QCOMPARE(QString::fromUtf8(VuoText_summaryFromValue(value.toUtf8().data())), summary);
	}

	void testLength_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<int>("length");

		QTest::newRow("empty string")		<< ""										<< 0;
		QTest::newRow("short")				<< "a"										<< 1;
		QTest::newRow("longer")				<< "0123456789"								<< 10;
		QTest::newRow("UTF8 short")			<< QString::fromUtf8("流")					<< 1;
		QTest::newRow("UTF8 longer")		<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨")	<< 10;
	}
	void testLength()
	{
		QFETCH(QString, value);
		QFETCH(int, length);

		QCOMPARE(VuoText_length(value.toUtf8().data()), (size_t)length);
	}

	void testSubstring_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<int>("startIndex");
		QTest::addColumn<int>("length");
		QTest::addColumn<QString>("substring");

		QTest::newRow("empty string")											<< ""								<< 1	<< 0	<< "";
		QTest::newRow("empty string, length too large")							<< ""								<< 1	<< 1	<< "";
		QTest::newRow("UTF8 string, first half")								<< QString::fromUtf8("⓪①②③④⑤")	<< 1	<< 3	<< QString::fromUtf8("⓪①②");
		QTest::newRow("UTF8 string, second half")								<< QString::fromUtf8("⓪①②③④⑤")	<< 4	<< 3	<< QString::fromUtf8("③④⑤");
		QTest::newRow("UTF8 string, all")										<< QString::fromUtf8("⓪①②③④⑤")	<< 1	<< 6	<< QString::fromUtf8("⓪①②③④⑤");
		QTest::newRow("UTF8 string, none")										<< QString::fromUtf8("⓪①②③④⑤")	<< 4	<< 0	<< "";
		QTest::newRow("UTF8 string, startIndex too large")						<< QString::fromUtf8("⓪①②③④⑤")	<< 7	<< 1	<< "";
		QTest::newRow("UTF8 string, startIndex zero")							<< QString::fromUtf8("⓪①②③④⑤")	<< 0	<< 3	<< QString::fromUtf8("⓪①");
		QTest::newRow("UTF8 string, startIndex negative, length reaches")		<< QString::fromUtf8("⓪①②③④⑤")	<< -1	<< 3	<< QString::fromUtf8("⓪");
		QTest::newRow("UTF8 string, startIndex negative, length doesn't reach")	<< QString::fromUtf8("⓪①②③④⑤")	<< -1	<< 2	<< "";
		QTest::newRow("UTF8 string, length too large")							<< QString::fromUtf8("⓪①②③④⑤")	<< 2	<< 6	<< QString::fromUtf8("①②③④⑤");
		QTest::newRow("UTF8 string, length negative")							<< QString::fromUtf8("⓪①②③④⑤")	<< 2	<< -1	<< "";
	}
	void testSubstring()
	{
		QFETCH(QString, value);
		QFETCH(int, startIndex);
		QFETCH(int, length);
		QFETCH(QString, substring);

		QCOMPARE(QString::fromUtf8(VuoText_substring(value.toUtf8().data(), startIndex, length)), substring);
	}

	void testAppend_data()
	{
		QTest::addColumn< QList<VuoText> >("texts");
		QTest::addColumn<QString>("compositeText");

		{
			QList<VuoText> list;
			QTest::newRow("empty list")	<< list << "";
		}
		{
			QList<VuoText> list;
			list.append("one");
			QTest::newRow("one item") << list << "one";
		}
		{
			QList<VuoText> list;
			list.append("first");
			list.append("");
			list.append("third");
			QTest::newRow("empty and non-empty items") << list << "firstthird";
		}
		{
			QList<VuoText> list;
			list.append("⓪");
			list.append("①②");
			QTest::newRow("UTF8 items") << list << "⓪①②";
		}
	}
	void testAppend()
	{
		QFETCH(QList<VuoText>, texts);
		QFETCH(QString, compositeText);

		unsigned long textsCount = texts.size();
		VuoText *textsArray = (VuoText *) malloc(textsCount * sizeof(VuoText));
		for (int i = 0; i < textsCount; ++i)
			textsArray[i] = texts.at(i);

		QCOMPARE(QString::fromUtf8(VuoText_append(textsArray, textsCount)), compositeText);
	}
};

QTEST_APPLESS_MAIN(TestVuoText)

#include "TestVuoText.moc"
