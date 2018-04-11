/**
 * @file
 * TestVuoText implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoText.h"
#include "VuoTextCase.h"
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

		QCOMPARE(QString::fromUtf8(VuoText_getSummary(NULL)), QString("<code>&#0;</code>"));

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

		QTest::newRow("empty string")		<< ""									<< "<code>&#0;</code>";
		QTest::newRow("short")				<< "a"									<< "<code>a</code>";
		QTest::newRow("borderline")			<< "01234567890123456789012345678901234567890123456789"	<< "<code>01234567890123456789012345678901234567890123456789</code>";
		QTest::newRow("UTF8 short")			<< QString::fromUtf8("流")				<< QString::fromUtf8("<code>流</code>");
		QTest::newRow("UTF8 long")          << QString::fromUtf8("⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾㊿") << QString::fromUtf8("<code>⓪①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟㊱㊲㊳㊴㊵㊶㊷㊸㊹㊺㊻㊼㊽㊾㊿</code>");
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

	void testEmpty_data()
	{
		QTest::addColumn<void *>("text");
		QTest::addColumn<bool>("expectedEmpty");

		QTest::newRow("NULL")		   << (void *)NULL << true;
		QTest::newRow("empty string")  << (void *)""   << true;
		QTest::newRow("space")         << (void *)" "  << false;
		QTest::newRow("letter")        << (void *)"a"  << false;
	}
	void testEmpty()
	{
		QFETCH(void *, text);
		QFETCH(bool, expectedEmpty);

		QCOMPARE(VuoText_isEmpty((VuoText)text), expectedEmpty);
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

	void testInsert_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<int>("startIndex");
		QTest::addColumn<QString>("newText");
		QTest::addColumn<QString>("result");

		QTest::newRow("emptyValue") 		<< "" 			<<  0	<< "Hello"		<< "Hello";
		QTest::newRow("emptyInsert") 		<< "Goodbye" 	<<  0	<< "" 			<< "Goodbye";
		QTest::newRow("endOfString") 		<< "Hello" 		<<  6	<< "Goodbye" 	<< "HelloGoodbye";
		QTest::newRow("startOfString") 		<< "Hello" 		<<  1	<< "Goodbye" 	<< "GoodbyeHello";
		QTest::newRow("beforeString") 		<< "Hello" 		<< -1	<< "Goodbye" 	<< "GoodbyeHello";
		QTest::newRow("afterString") 		<< "Hello" 		<< 10	<< "Goodbye" 	<< "HelloGoodbye";
		QTest::newRow("middleOfString")		<< "Hello" 		<<  2	<< "Goodbye" 	<< "HGoodbyeello";
		QTest::newRow("middleOfString2")	<< "Hello" 		<<  5	<< "Goodbye" 	<< "HellGoodbyeo";
	}

	void testInsert()
	{
		QFETCH(QString, value);
		QFETCH(int, startIndex);
		QFETCH(QString, newText);
		QFETCH(QString, result);

		VuoText ins = VuoText_insert( (const char*)value.toUtf8().data(), startIndex, (const char*)newText.toUtf8().data());
		VuoRetain(ins);

		QCOMPARE(QString::fromUtf8(ins), result);

		VuoRelease(ins);
	}

	void testConvertUtf32_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<unsigned int>("decimalValue");

		QTest::newRow("ASCII: a") 			<< "a"	<< (unsigned int)97;
		QTest::newRow("ASCII: H") 			<< "H"	<< (unsigned int)72;
		QTest::newRow("OSX New Line") 		<< "\n"	<< (unsigned int)10;
		QTest::newRow("Windows New Line") 	<< "\r"	<< (unsigned int)13;
		QTest::newRow("Diacritics é") 		<< "é"	<< (unsigned int)233;
		QTest::newRow("Diacritics ü") 		<< "ü"	<< (unsigned int)252;
		QTest::newRow("Chess Knight") 		<< "♞"	<< (unsigned int)9822;
		QTest::newRow("Space") 				<< " "	<< (unsigned int)32;

		QTest::newRow("Sentence") 			<< "Héllo, this is an über séntence\twith\nweird characters like ∑ and Ѩ.\n" << (unsigned int)72;
	}

	void testConvertUtf32()
	{
		QFETCH(QString, value);
		QFETCH(unsigned int, decimalValue);

		VuoText text = VuoText_make( (const char*)value.toUtf8().data() );
		VuoLocal(text);

		size_t len;
		uint32_t* buffer = VuoText_getUtf32Values(text, &len);

		QCOMPARE(buffer[0], decimalValue);

		VuoText converted = VuoText_makeFromUtf32(buffer, len);
		VuoLocal(converted);
		free(buffer);

		QCOMPARE(QString::fromUtf8(converted), value);
	}

	void testConvertMacRoman_data()
	{
		QTest::addColumn<void *>("macRoman");
		QTest::addColumn<void *>("utf8");

		// https://b33p.net/kosada/node/12507
		QTest::newRow("French Audio Output") << (void *)"Sortie int\216gr\216e" << (void *)"Sortie intégrée";
	}
	void testConvertMacRoman()
	{
		QFETCH(void *, macRoman);
		QFETCH(void *, utf8);

		VuoText text = VuoText_makeFromMacRoman((const char *)macRoman);
		QVERIFY(text);
		VuoLocal(text);

		QVERIFY(VuoText_areEqual(text, (VuoText)utf8));
	}

	void testRemoveAt_data()
	{
		QTest::addColumn<QString>("value");
		QTest::addColumn<int>("startIndex");
		QTest::addColumn<int>("length");
		QTest::addColumn<QString>("result");

		QTest::newRow("emptyValue") 		<< "" 			<<  0  <<  1 << "";
		QTest::newRow("noLength") 			<< "Hello" 		<<  1  <<  0 << "Hello";
		QTest::newRow("negativeLength") 	<< "Hello" 		<<  1  <<  -3 << "Hello";
		QTest::newRow("removeMultiple") 	<< "Hello" 		<<  1  <<  3 << "lo";
		QTest::newRow("firstLetter") 		<< "Hello" 		<<  1  <<  1 << "ello";
		QTest::newRow("middleChar") 		<< "Hello" 		<<  3  <<  1 << "Helo";
		QTest::newRow("lastChar") 			<< "Hello" 		<<  5  <<  1 << "Hell";
		QTest::newRow("beforeFirst1Len")	<< "Hello" 		<<  -1  <<  1 << "Hello";
		QTest::newRow("beforeFirst3Len")	<< "Hello" 		<<  -1  <<  3 << "ello";
		QTest::newRow("outOfBounds")		<< "Hello" 		<<  10  <<  2 << "Hello";
		QTest::newRow("allCharacters")		<< "Hello" 		<<  1  <<  5 << "";
	}

	void testRemoveAt()
	{
		QFETCH(QString, value);
		QFETCH(int, startIndex);
		QFETCH(int, length);
		QFETCH(QString, result);

		VuoText rm = VuoText_removeAt( (const char*)value.toUtf8().data(), startIndex, length);
		VuoLocal(rm);

		QCOMPARE(QString::fromUtf8(rm), result);
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

	void testLessThan_data()
	{
		QTest::addColumn<void *>("text1");
		QTest::addColumn<void *>("text2");
		QTest::addColumn<bool>("expectedLessThan");

		QTest::newRow("both NULL")						<< (void *)NULL	<< (void *)NULL		<< false;

		// NULL should be less than any string.
		QTest::newRow("NULL, emptystring")				<< (void *)NULL	<< (void *)""		<< true;
		QTest::newRow("NULL, string")					<< (void *)NULL	<< (void *)"foo"	<< true;
		QTest::newRow("emptystring, NULL")				<< (void *)""	<< (void *)NULL		<< false;
		QTest::newRow("emptystring, emptystring")		<< (void *)""	<< (void *)""		<< false;

		// Emptystring should be less than any non-emptystring.
		QTest::newRow("emptystring, string")			<< (void *)""	<< (void *)"foo"	<< true;
		QTest::newRow("string, emptystring")			<< (void *)"foo"<< (void *)""		<< false;

		// Ensure Unicode comparison works.
		QTest::newRow("different strings 1")			<< (void *)"①"	<< (void *)"②"		<< true;
		QTest::newRow("different strings 2")			<< (void *)"②"	<< (void *)"①"		<< false;
		QTest::newRow("same strings, same encoding")	<< (void *)"⓪"	<< (void *)"⓪"		<< false;

		{
			// http://en.wikipedia.org/wiki/Combining_character
			const QChar cyrillicU_combiningBreve[] = { QChar(0x0423), QChar(0x0306) };
			QString stringAsDecomposedCharacters(cyrillicU_combiningBreve, 2);
			const QChar cyrillicShortU(0x040E);
			QString stringAsComposedCharacter(cyrillicShortU);
			void *decomposed = strdup(stringAsDecomposedCharacters.toUtf8().data());
			void *composed = strdup(stringAsComposedCharacter.toUtf8().data());

			// Different Unicode compositions should be treated as equal (not less than or greater than).
			QTest::newRow("same strings, different encoding 0") << decomposed << composed << false;
			QTest::newRow("same strings, different encoding 1") << composed << decomposed << false;
		}

		// http://www.thai-language.com/ref/alphabetical-order
		QTest::newRow("Thai <")					<< (void *)"สงวน"		<< (void *)"สงสัย"		<< true;
		QTest::newRow("Thai >")					<< (void *)"สงสัย"		<< (void *)"สงวน"		<< false;

		// https://en.wikipedia.org/wiki/Goj%C5%ABon
		QTest::newRow("Japanese hiragana <")	<< (void *)"あ"			<< (void *)"い"			<< true;
		QTest::newRow("Japanese hiragana >")	<< (void *)"い"			<< (void *)"あ"			<< false;

		// https://en.wikipedia.org/wiki/Chinese_characters#Indexing
		QTest::newRow("Chinese radical <")		<< (void *)"妈"			<< (void *)"松"			<< true;
		QTest::newRow("Chinese radical >")		<< (void *)"松"			<< (void *)"妈"			<< false;

		// Old Danish sorts "Å" at the end — http://boards.straightdope.com/sdmb/showpost.php?p=14289219&postcount=14
		QTest::newRow("Danish <")				<< (void *)"København"	<< (void *)"Århus"		<< true;
		QTest::newRow("Danish >")				<< (void *)"Århus"		<< (void *)"København"	<< false;
	}
	void testLessThan()
	{
		QFETCH(void *, text1);
		QFETCH(void *, text2);
		QFETCH(bool, expectedLessThan);

		QEXPECT_FAIL("Danish <", "CFStringCompare() doesn't seem to honor this convention.", Continue);
		QEXPECT_FAIL("Danish >", "CFStringCompare() doesn't seem to honor this convention.", Continue);

		QCOMPARE(VuoText_isLessThan((VuoText)text1, (VuoText)text2), expectedLessThan);
	}

	void testCompare_data()
	{
		QTest::addColumn<void *>("text1");
		QTest::addColumn<void *>("text2");
		QTest::addColumn<bool>("expectedEqualsCase");		// case-sensitive
		QTest::addColumn<bool>("expectedContainsCase");		//
		QTest::addColumn<bool>("expectedBeginsWithCase");	//
		QTest::addColumn<bool>("expectedEndsWithCase");		//
		QTest::addColumn<bool>("expectedEquals");			// not case-sensitive
		QTest::addColumn<bool>("expectedContains");			//
		QTest::addColumn<bool>("expectedBeginsWith");		//
		QTest::addColumn<bool>("expectedEndsWith");			//

		QTest::newRow("both null")						<< (void*)NULL				<< (void *)NULL				<< true << true << true << true << true << true << true << true;
		QTest::newRow("first null")						<< (void*)NULL				<< (void*)"cat"				<< false << false << false << false << false << false << false << false;
		QTest::newRow("second null")					<< (void*)"cat"				<< (void*)NULL				<< false << true << true << true << false << true << true << true;
		QTest::newRow("both empty")						<< (void*)""				<< (void*)""				<< true << true << true << true << true << true << true << true;
		QTest::newRow("first empty")					<< (void*)""				<< (void*)"cat"				<< false << false << false << false << false << false << false << false;
		QTest::newRow("second empty")					<< (void*)"cat"				<< (void*)""				<< false << true << true << true << false << true << true << true;
		QTest::newRow("same")							<< (void *)"Raison d'être"	<< (void *)"Raison d'être"	<< true << true << true << true << true << true << true << true;
		QTest::newRow("first begins with second")		<< (void *)"你好吗"			<< (void *)"你好"			<< false << true << true << false << false << true << true << false;
		QTest::newRow("first ends with second")			<< (void *)"nærmiljø"		<< (void *)"miljø"			<< false << true << false << true << false << true << false << true;
		QTest::newRow("first contains second")			<< (void *)"¿Dónde estás?"	<< (void *)"está"			<< false << true << false << false << false << true << false << false;
		QTest::newRow("same except case")				<< (void *)"AtÉ AmanhÃ."	<< (void *)"aTé aMANHã."	<< false << false << false << false << true << true << true << true;
		QTest::newRow("different first character")		<< (void *)"①②③④⑤"			<< (void *)"1②③④⑤"			<< false << false << false << false << false << false << false << false;
		QTest::newRow("different last character")		<< (void *)"①②③④⑤"			<< (void *)"①②③④5"			<< false << false << false << false << false << false << false << false;
	}
	void testCompare()
	{
		QFETCH(void *, text1);
		QFETCH(void *, text2);
		QFETCH(bool, expectedEqualsCase);
		QFETCH(bool, expectedContainsCase);
		QFETCH(bool, expectedBeginsWithCase);
		QFETCH(bool, expectedEndsWithCase);
		QFETCH(bool, expectedEquals);
		QFETCH(bool, expectedContains);
		QFETCH(bool, expectedBeginsWith);
		QFETCH(bool, expectedEndsWith);

		bool expected[2][4] = { { expectedEqualsCase, expectedContainsCase, expectedBeginsWithCase, expectedEndsWithCase },
								{ expectedEquals, expectedContains, expectedBeginsWith, expectedEndsWith } };
		VuoTextComparisonType types[] = { VuoTextComparison_Equals, VuoTextComparison_Contains, VuoTextComparison_BeginsWith, VuoTextComparison_EndsWith };

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				VuoTextComparison comparison = { types[j], 1-i };
				QVERIFY2(VuoText_compare((VuoText)text1, comparison, (VuoText)text2) == expected[i][j], VuoTextComparison_getSummary(comparison));
			}
		}
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

	void testCapitalization_data()
	{
		QTest::addColumn<QString>("sentence");
		QTest::addColumn<QString>("lowercaseAll");
		QTest::addColumn<QString>("uppercaseAll");
		QTest::addColumn<QString>("uppercaseFirstLetterWord");
		QTest::addColumn<QString>("uppercaseFirstLetterSentence");

		QTest::newRow("Mixed Case") 			<< "loRem IPsum dolor? moRE words." 													<< "lorem ipsum dolor? more words." 													<< "LOREM IPSUM DOLOR? MORE WORDS." 													<< "Lorem Ipsum Dolor? More Words." 													<< "Lorem ipsum dolor? More words.";
		QTest::newRow("Mixed Case w/ Symbols") 	<< "$$$sOme (words) in THE MIDdl&e.  ANOTHER SENTeNcE!"									<< "$$$some (words) in the middl&e.  another sentence!" 								<< "$$$SOME (WORDS) IN THE MIDDL&E.  ANOTHER SENTENCE!" 								<< "$$$Some (Words) In The Middl&E.  Another Sentence!" 								<< "$$$some (words) in the middl&e.  Another sentence!";
		// The sentence-case does not capitalize the first 'f' in Feliz Cumpleaños due to locale settings.
		QTest::newRow("Diacritics") 			<< "¡Feliz cumpleaños! Fußgängerübergänge means \"Pedestrian crossings\" in English." 	<< "¡feliz cumpleaños! fußgängerübergänge means \"pedestrian crossings\" in english." 	<< "¡FELIZ CUMPLEAÑOS! FUSSGÄNGERÜBERGÄNGE MEANS \"PEDESTRIAN CROSSINGS\" IN ENGLISH." 	<< "¡Feliz Cumpleaños! Fußgängerübergänge Means \"Pedestrian Crossings\" In English." 	<< "¡feliz cumpleaños! Fußgängerübergänge means \"pedestrian crossings\" in english.";
	}

	void testCapitalization()
	{
		QFETCH(QString, sentence);
		QFETCH(QString, lowercaseAll);
		QFETCH(QString, uppercaseAll);
		QFETCH(QString, uppercaseFirstLetterWord);
		QFETCH(QString, uppercaseFirstLetterSentence);

		VuoText v_lowercaseAll = VuoText_changeCase(sentence.toUtf8().data(), VuoTextCase_LowercaseAll);
		VuoText v_uppercaseAll = VuoText_changeCase(sentence.toUtf8().data(), VuoTextCase_UppercaseAll);
		VuoText v_uppercaseFirstLetterWord = VuoText_changeCase(sentence.toUtf8().data(), VuoTextCase_UppercaseFirstLetterWord);
		VuoText v_uppercaseFirstLetterSentence = VuoText_changeCase(sentence.toUtf8().data(), VuoTextCase_UppercaseFirstLetterSentence);

		if(v_lowercaseAll != NULL) VuoRetain(v_lowercaseAll);
		if(v_uppercaseAll != NULL) VuoRetain(v_uppercaseAll);
		if(v_uppercaseFirstLetterWord != NULL) VuoRetain(v_uppercaseFirstLetterWord);
		if(v_uppercaseFirstLetterSentence != NULL) VuoRetain(v_uppercaseFirstLetterSentence);

		QVERIFY( VuoText_areEqual(lowercaseAll.toUtf8().data(), v_lowercaseAll) );
		QVERIFY( VuoText_areEqual(uppercaseAll.toUtf8().data(), v_uppercaseAll) );
		QVERIFY( VuoText_areEqual(uppercaseFirstLetterWord.toUtf8().data(), v_uppercaseFirstLetterWord) );
		QVERIFY( VuoText_areEqual(uppercaseFirstLetterSentence.toUtf8().data(), v_uppercaseFirstLetterSentence) );

		if(v_lowercaseAll != NULL) VuoRelease(v_lowercaseAll);
		if(v_uppercaseAll != NULL) VuoRelease(v_uppercaseAll);
		if(v_uppercaseFirstLetterWord != NULL) VuoRelease(v_uppercaseFirstLetterWord);
		if(v_uppercaseFirstLetterSentence != NULL) VuoRelease(v_uppercaseFirstLetterSentence);

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
			unsigned char *bad = (unsigned char *)malloc(1);
			// Byte 0xfe is not allowed in UTF-8.
			bad[0] = 0xfe;

			VuoData badData = VuoData_make(sizeof(bad), (unsigned char *)bad);
			VuoData_retain(badData);
			QTest::newRow("bad: solo 0xfe") << (void *)VuoData_getJson(badData) << "";
			VuoData_release(badData);
		}

		{
			unsigned char *bad = (unsigned char *)malloc(1);
			// Byte 0xff is not allowed in UTF-8.
			bad[0] = 'h';
			bad[1] = 'i';
			bad[2] = 0xff;
			bad[3] = 'o';
			bad[4] = 'k';

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
