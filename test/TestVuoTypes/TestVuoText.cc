/**
 * @file
 * TestVuoText implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoText.h"
#include "VuoData.h"
#include <json-c/json.h>
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(VuoText);
Q_DECLARE_METATYPE(void *);

/**
 * Tests the VuoText type.
 */
class TestVuoText : public QObject
{
	Q_OBJECT

private slots:

	void testNull()
	{
		// Test NULL strings separately, since you can't pass a NULL QString as data into the other test methods.

		QCOMPARE(QString::fromUtf8(VuoText_make(NULL)), QString(""));

		QCOMPARE(QString::fromUtf8(VuoText_getString(NULL)), QString("null"));

		QCOMPARE(QString::fromUtf8(VuoText_getSummary(NULL)), QString("<code></code>"));

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

		QTest::newRow("empty string")		<< ""									<< "<code></code>";
		QTest::newRow("short")				<< "a"									<< "<code>a</code>";
		QTest::newRow("borderline")			<< "01234567890123456789012345678901234567890123456789"	<< "<code>01234567890123456789012345678901234567890123456789</code>";
		QTest::newRow("long")				<< "012345678901234567890123456789012345678901234567890"	<< "<code>01234567890123456789012345678901234567890123456789…</code>";
		QTest::newRow("UTF8 short")			<< QString::fromUtf8("流")				<< QString::fromUtf8("<code>流</code>");
		QTest::newRow("UTF8 borderline")	<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾")	<< QString::fromUtf8("<code>⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾</code>");
		QTest::newRow("UTF8 long")		<< QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾㊿")	<< QString::fromUtf8("<code>⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾…</code>");
	}
	void testSummary()
	{
		QFETCH(QString, value);
		QFETCH(QString, summary);

		QCOMPARE(QString::fromUtf8(VuoText_getSummary(value.toUtf8().data())), summary);
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
		QTest::newRow("UTF8 string, startIndex negative, barely doesn't reach")	<< QString::fromUtf8("⓪①②③④⑤")	<< -1	<< 2	<< "";
		QTest::newRow("UTF8 string, startIndex negative, doesn't reach")		<< QString::fromUtf8("⓪①②③④⑤")	<< -1	<< 1	<< "";
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
		QTest::addColumn<void *>("text1");
		QTest::addColumn<void *>("text2");
		QTest::addColumn<bool>("expectedEqual");

		QTest::newRow("both NULL")						<< (void *)NULL	<< (void *)NULL		<< true;
		QTest::newRow("NULL, emptystring")				<< (void *)NULL	<< (void *)""		<< false;
		QTest::newRow("NULL, string")					<< (void *)NULL	<< (void *)"foo"	<< false;
		QTest::newRow("emptystring, NULL")				<< (void *)""	<< (void *)NULL		<< false;
		QTest::newRow("emptystring, emptystring")		<< (void *)""	<< (void *)""		<< true;
		QTest::newRow("different strings")				<< (void *)"⓪"	<< (void *)"①"		<< false;
		QTest::newRow("same strings, same encoding")	<< (void *)"⓪"	<< (void *)"⓪"		<< true;

		{
			// http://en.wikipedia.org/wiki/Combining_character
			const QChar cyrillicU_combiningBreve[] = { QChar(0x0423), QChar(0x0306) };
			QString stringAsDecomposedCharacters(cyrillicU_combiningBreve, 2);
			const QChar cyrillicShortU(0x040E);
			QString stringAsComposedCharacter(cyrillicShortU);
			QTest::newRow("same strings, different encoding") << (void *)strdup(stringAsDecomposedCharacters.toUtf8().data()) << (void *)strdup(stringAsComposedCharacter.toUtf8().data()) << true;
		}
	}
	void testEqual()
	{
		QFETCH(void *, text1);
		QFETCH(void *, text2);
		QFETCH(bool, expectedEqual);

		QCOMPARE(VuoText_areEqual((VuoText)text1, (VuoText)text2), expectedEqual);
	}

	void testTrim_data()
	{
		QTest::addColumn<void *>("text");
		QTest::addColumn<void *>("expectedText");

		QTest::newRow("NULL")						<< (void *)NULL		<< (void *)NULL;
		QTest::newRow("emptystring")				<< (void *)""		<< (void *)"";
		QTest::newRow("all whitespace")				<< (void *)" "		<< (void *)"";
		QTest::newRow("all whitespace 2")			<< (void *)"      "	<< (void *)"";
		QTest::newRow("no whitespace")				<< (void *)"foo"	<< (void *)"foo";
		QTest::newRow("inner whitespace")			<< (void *)"a b"	<< (void *)"a b";
		QTest::newRow("leading whitespace")			<< (void *)" a"		<< (void *)"a";
		QTest::newRow("trailing whitespace")		<< (void *)"a "		<< (void *)"a";
		QTest::newRow("both whitespace")			<< (void *)" a "	<< (void *)"a";
		QTest::newRow("both whitespace 2")			<< (void *)"   a  "	<< (void *)"a";
	}
	void testTrim()
	{
		QFETCH(void *, text);
		QFETCH(void *, expectedText);

		QCOMPARE(VuoText_trim((VuoText)text), (VuoText)expectedText);
	}

	void testFind_data()
	{
		QTest::addColumn<QString>("string");
		QTest::addColumn<QString>("substring");
		QTest::addColumn<size_t>("expectedFirstIndex");
		QTest::addColumn<size_t>("expectedLastIndex");

		QTest::newRow("Not found")				<< "⓪①②③④" << "⑤"		<< (size_t)0 << (size_t)0;
		QTest::newRow("Found at beginning")		<< "⓪①②③④" << "⓪"		<< (size_t)1 << (size_t)1;
		QTest::newRow("Found emptystring")		<< "⓪①②③④" << ""		<< (size_t)1 << (size_t)6;
		QTest::newRow("Found empty-empty")		<< ""			<< ""		<< (size_t)1 << (size_t)1;
		QTest::newRow("Found empty-NULL")		<< ""			<< "NULL"	<< (size_t)1 << (size_t)1;
		QTest::newRow("Found NULL-empty")		<< "NULL"		<< ""		<< (size_t)1 << (size_t)1;
		QTest::newRow("Found NULL-NULL")		<< "NULL"		<< "NULL"	<< (size_t)1 << (size_t)1;
		QTest::newRow("Found at end")			<< "⓪①②③④" << "④"		<< (size_t)5 << (size_t)5;
		QTest::newRow("Multiple occurrences")	<< "⓪①①①④" << "①"		<< (size_t)2 << (size_t)4;
		QTest::newRow("Multiple characters")	<< "⓪①②③④" << "①②③④"	<< (size_t)2 << (size_t)2;
	}
	void testFind()
	{
		QFETCH(QString, string);
		QFETCH(QString, substring);
		QFETCH(size_t, expectedFirstIndex);
		QFETCH(size_t, expectedLastIndex);

		VuoText    stringT =    string == "NULL" ? NULL : VuoText_make(   string.toUtf8().data());
		VuoText substringT = substring == "NULL" ? NULL : VuoText_make(substring.toUtf8().data());

		QCOMPARE(VuoText_findFirstOccurrence(stringT, substringT, 1), expectedFirstIndex);
		QCOMPARE(VuoText_findLastOccurrence(stringT, substringT), expectedLastIndex);
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

	void testFromData_data()
	{
		QTest::addColumn<void *>("jsonData");
		QTest::addColumn<QString>("expectedText");

		QTest::newRow("null") << (void *)NULL << "";

		{
			const char *hello = "Héllo Wørld!";
			VuoData helloData = VuoData_makeFromText(hello);
			VuoData_retain(helloData);
			QTest::newRow("hello") << (void *)VuoData_getJson(helloData) << hello;
			VuoData_release(helloData);
		}

		{
			// Byte 0xfe is not allowed in UTF-8.
			const unsigned char bad[] = { 0xfe };

			VuoData badData = VuoData_make(sizeof(bad), (unsigned char *)bad);
			VuoData_retain(badData);
			QTest::newRow("bad: solo 0xfe") << (void *)VuoData_getJson(badData) << "";
			VuoData_release(badData);
		}

		{
			// Byte 0xff is not allowed in UTF-8.
			const unsigned char bad[] = { 'h', 'i', 0xff, 'o', 'k' };

			VuoData badData = VuoData_make(sizeof(bad), (unsigned char *)bad);
			VuoData_retain(badData);
			QTest::newRow("bad: midstream 0xff") << (void *)VuoData_getJson(badData) << "";
			VuoData_release(badData);
		}
	}
	void testFromData()
	{
		QFETCH(void *, jsonData);
		QFETCH(QString, expectedText);

		VuoData data = VuoData_makeFromJson((json_object *)jsonData);
		VuoData_retain(data);
		VuoText text = VuoText_makeFromData((unsigned char *)data.data, data.size);
		VuoRetain(text);

		QCOMPARE(QString(text), expectedText);

		VuoRelease(text);
		VuoData_release(data);
	}
};

QTEST_APPLESS_MAIN(TestVuoText)

#include "TestVuoText.moc"
