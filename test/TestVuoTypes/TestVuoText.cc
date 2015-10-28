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
		QTest::newRow("borderline")			<< "01234567890123456789012345678901234567890123456789"	<< "01234567890123456789012345678901234567890123456789";
		QTest::newRow("long")				<< "012345678901234567890123456789012345678901234567890"	<< "01234567890123456789012345678901234567890123456789...";
		QTest::newRow("UTF8 short")			<< QString::fromUtf8("流")				<< QString::fromUtf8("流");
		QTest::newRow("UTF8 borderline")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾");
		QTest::newRow("UTF8 long")		<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾㊿")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾...");
	}
	void testSummary()
	{
		QFETCH(QString, value);
		QFETCH(QString, summary);

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

	void testEqual_data()
	{
		QTest::addColumn<QString>("text1");
		QTest::addColumn<QString>("text2");
		QTest::addColumn<bool>("expectedEqual");

		{
			QTest::newRow("Different strings") << "⓪" << "①" << false;
		}
		{
			QTest::newRow("Same strings, same encoding") << "⓪" << "⓪" << true;
		}
		{
			// http://en.wikipedia.org/wiki/Combining_character
			const QChar cyrillicU_combiningBreve[] = { QChar(0x0423), QChar(0x0306) };
			QString stringAsDecomposedCharacters(cyrillicU_combiningBreve, 2);
			const QChar cyrillicShortU(0x040E);
			QString stringAsComposedCharacter(cyrillicShortU);
			QTest::newRow("Same strings, different encoding") << stringAsDecomposedCharacters << stringAsComposedCharacter << true;
		}
	}
	void testEqual()
	{
		QFETCH(QString, text1);
		QFETCH(QString, text2);
		QFETCH(bool, expectedEqual);

		QCOMPARE(VuoText_areEqual(text1.toUtf8().data(), text2.toUtf8().data()), expectedEqual);
	}

	void testFind_data()
	{
		QTest::addColumn<QString>("string");
		QTest::addColumn<QString>("substring");
		QTest::addColumn<size_t>("expectedIndex");

		{
			QTest::newRow("Not found") << "⓪①②③④" << "⑤" << (size_t)0;
		}
		{
			QTest::newRow("Found at beginning") << "⓪①②③④" << "⓪" << (size_t)1;
		}
		{
			QTest::newRow("Found at end") << "⓪①②③④" << "④" << (size_t)5;
		}
		{
			QTest::newRow("Multiple occurrences") << "⓪①①①④" << "①" << (size_t)4;
		}
		{
			QTest::newRow("Multiple characters") << "⓪①②③④" << "①②③④" << (size_t)2;
		}
	}
	void testFind()
	{
		QFETCH(QString, string);
		QFETCH(QString, substring);
		QFETCH(size_t, expectedIndex);

		QCOMPARE(VuoText_findLastOccurrence(string.toUtf8().data(), substring.toUtf8().data()), expectedIndex);
	}

	void testReplace_data()
	{
		QTest::addColumn<QString>("subject");
		QTest::addColumn<QString>("stringToFind");
		QTest::addColumn<QString>("replacement");
		QTest::addColumn<QString>("expectedReplacedString");

		QTest::newRow("Not found")	<< "⓪①②③④" << "⑤" << "⓪" << "⓪①②③④";
		QTest::newRow("Single")		<< "⓪①②③④" << "③" << "⑤" << "⓪①②⑤④";
		QTest::newRow("Multiple")	<< "⓪⓪①①②" << "①" << "⑤" << "⓪⓪⑤⑤②";
	}
	void testReplace()
	{
		QFETCH(QString, subject);
		QFETCH(QString, stringToFind);
		QFETCH(QString, replacement);
		QFETCH(QString, expectedReplacedString);

		QVERIFY(VuoText_areEqual(VuoText_replace(subject.toUtf8().data(), stringToFind.toUtf8().data(), replacement.toUtf8().data()), expectedReplacedString.toUtf8().data()));
	}
};

QTEST_APPLESS_MAIN(TestVuoText)

#include "TestVuoText.moc"
