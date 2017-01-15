/**
 * @file
 * TestVuoList implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoText.h"
#include "VuoList_VuoText.h"
}

// Be able to use this type in QTest::addColumn()
Q_DECLARE_METATYPE(char *);
Q_DECLARE_METATYPE(VuoList_VuoText);

/**
 * Tests the VuoList_VuoText type (no need to test other list types, since they're all automatically generated from the same source).
 */
class TestVuoList : public QObject
{
	Q_OBJECT

private slots:

	void testList()
	{
		VuoList_VuoText l = VuoListCreate_VuoText();
		QCOMPARE(VuoListGetCount_VuoText(l), 0UL);

		VuoListAppendValue_VuoText(l, VuoText_make("zero"));
		QCOMPARE(VuoListGetCount_VuoText(l), 1UL);

		VuoListAppendValue_VuoText(l, VuoText_make("one"));
		QCOMPARE(VuoListGetCount_VuoText(l), 2UL);

		VuoListAppendValue_VuoText(l, VuoText_make("two"));
		QCOMPARE(VuoListGetCount_VuoText(l), 3UL);

		QCOMPARE(QString(VuoListGetValue_VuoText(l,1)), QString("zero"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,2)), QString("one"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,3)), QString("two"));

		{
			char *lSerialized = VuoList_VuoText_getString(l);
			VuoList_VuoText lUnserialized = VuoList_VuoText_makeFromString(lSerialized);
			QCOMPARE(VuoListGetCount_VuoText(l), VuoListGetCount_VuoText(lUnserialized));
			unsigned long itemCount = VuoListGetCount_VuoText(l);
			for (unsigned long i = 1; i <= itemCount; ++i)
			{
				VuoText lItem = VuoListGetValue_VuoText(l, i);
				VuoText lUnserializedItem = VuoListGetValue_VuoText(lUnserialized, i);
				QCOMPARE(QString(lItem), QString(lUnserializedItem));
			}
			free(lSerialized);
		}

		{
			VuoListSort_VuoText(l);
			QCOMPARE(QString(VuoListGetValue_VuoText(l,1)), QString("one"));
			QCOMPARE(QString(VuoListGetValue_VuoText(l,2)), QString("two"));
			QCOMPARE(QString(VuoListGetValue_VuoText(l,3)), QString("zero"));
		}
	}

	bool isItemPresent(VuoList_VuoText l, QString s)
	{
		VuoInteger count = VuoListGetCount_VuoText(l);
		for (VuoInteger i = 1; i <= count; ++i)
			if (VuoListGetValue_VuoText(l, i) == s)
				return true;

		return false;
	}

	void verifyEqual(VuoList_VuoText a, VuoList_VuoText b)
	{
		QCOMPARE(VuoListGetCount_VuoText(a), VuoListGetCount_VuoText(b));

		unsigned long size = VuoListGetCount_VuoText(a);
		for (unsigned long i = 1; i <= size; ++i)
			QCOMPARE(VuoListGetValue_VuoText(a, i), VuoListGetValue_VuoText(b, i));
	}

	void testInsert_data()
	{
		VuoText badger = VuoText_make("badger");
		VuoText mushroom = VuoText_make("mushroom");

		QTest::addColumn<VuoList_VuoText>("initialList");
		QTest::addColumn<QString>("item");
		QTest::addColumn<int>("index");
		QTest::addColumn<VuoList_VuoText>("expectedList");

		VuoList_VuoText mushroomList = VuoListCreate_VuoText();
		VuoListAppendValue_VuoText(mushroomList, mushroom);

		QTest::newRow("empty list @ 0")					<< VuoListCreate_VuoText() << mushroom << 0 << mushroomList;
		QTest::newRow("empty list @ 1")					<< VuoListCreate_VuoText() << mushroom << 1 << mushroomList;
		QTest::newRow("empty list @ 2")					<< VuoListCreate_VuoText() << mushroom << 2 << mushroomList;

		{
			VuoList_VuoText l = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(l, badger);
			VuoListAppendValue_VuoText(l, badger);

			VuoList_VuoText first = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(first, mushroom);
			VuoListAppendValue_VuoText(first, badger);
			VuoListAppendValue_VuoText(first, badger);

			VuoList_VuoText middle = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(middle, badger);
			VuoListAppendValue_VuoText(middle, mushroom);
			VuoListAppendValue_VuoText(middle, badger);

			VuoList_VuoText last = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(last, badger);
			VuoListAppendValue_VuoText(last, badger);
			VuoListAppendValue_VuoText(last, mushroom);

			QTest::newRow("2-item list @ 0")						<< l << mushroom << 0 << first;
			QTest::newRow("2-item list @ 1")						<< l << mushroom << 1 << first;
			QTest::newRow("2-item list @ 2")						<< l << mushroom << 2 << middle;
			QTest::newRow("2-item list @ 3")						<< l << mushroom << 3 << last;
			QTest::newRow("2-item list @ 4")						<< l << mushroom << 4 << last;
		}
	}
	void testInsert()
	{
		QFETCH(VuoList_VuoText, initialList);
		QFETCH(QString, item);
		QFETCH(int, index);
		QFETCH(VuoList_VuoText, expectedList);

		VuoList_VuoText actualList = VuoListCopy_VuoText(initialList);
		VuoListInsertValue_VuoText(actualList, VuoText_make(item.toUtf8().data()), index);

		verifyEqual(actualList, expectedList);
	}

	void testShuffle()
	{
		VuoList_VuoText l = VuoListCreate_VuoText();
		VuoListAppendValue_VuoText(l, VuoText_make("zero"));
		VuoListAppendValue_VuoText(l, VuoText_make("one"));
		VuoListAppendValue_VuoText(l, VuoText_make("two"));

		// Shuffling with chaos=0 shouldn't change the list.
		VuoListShuffle_VuoText(l, 0);
		QCOMPARE(QString(VuoListGetValue_VuoText(l,1)), QString("zero"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,2)), QString("one"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,3)), QString("two"));

		// A shuffled list should still have all the items.
		VuoListShuffle_VuoText(l, 1);
		QVERIFY(isItemPresent(l, "zero"));
		QVERIFY(isItemPresent(l, "one"));
		QVERIFY(isItemPresent(l, "two"));
	}

	void testReverse()
	{
		VuoList_VuoText l = VuoListCreate_VuoText();
		VuoListAppendValue_VuoText(l, VuoText_make("zero"));
		VuoListAppendValue_VuoText(l, VuoText_make("one"));
		VuoListAppendValue_VuoText(l, VuoText_make("two"));

		VuoListReverse_VuoText(l);
		QCOMPARE(QString(VuoListGetValue_VuoText(l,1)), QString("two"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,2)), QString("one"));
		QCOMPARE(QString(VuoListGetValue_VuoText(l,3)), QString("zero"));
	}

	void testCut_data()
	{
		QTest::addColumn<VuoList_VuoText>("initialList");
		QTest::addColumn<int>("startIndex");
		QTest::addColumn<int>("itemCount");
		QTest::addColumn<VuoList_VuoText>("expectedCutList");

		QTest::newRow("empty list")					<< VuoListCreate_VuoText() << 1 << 0 << VuoListCreate_VuoText();
		QTest::newRow("empty list, staring later")	<< VuoListCreate_VuoText() << 2 << 0 << VuoListCreate_VuoText();
		QTest::newRow("empty list, multiple")		<< VuoListCreate_VuoText() << 2 << 4 << VuoListCreate_VuoText();

		{
			VuoList_VuoText l = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(l, VuoText_make("one"));
			VuoListAppendValue_VuoText(l, VuoText_make("two"));
			VuoListAppendValue_VuoText(l, VuoText_make("three"));

			VuoList_VuoText first = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(first, VuoText_make("one"));

			VuoList_VuoText secondTwo = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(secondTwo, VuoText_make("two"));
			VuoListAppendValue_VuoText(secondTwo, VuoText_make("three"));

			VuoList_VuoText third = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(third, VuoText_make("three"));

			QTest::newRow("3-item list, get none")						<< l <<  1 <<  0 << VuoListCreate_VuoText();
			QTest::newRow("3-item list, get none, overrun")				<< l <<  4 << 42 << VuoListCreate_VuoText();
			QTest::newRow("3-item list, get none, negative overrun")	<< l << -9 <<  3 << VuoListCreate_VuoText();

			QTest::newRow("3-item list, get all")						<< l <<  1 <<  3 << l;
			QTest::newRow("3-item list, get all, overrun")				<< l <<  1 << 42 << l;
			QTest::newRow("3-item list, get all, zero start")			<< l <<  0 <<  4 << l;
			QTest::newRow("3-item list, get all, negative start")		<< l << -1 <<  5 << l;

			QTest::newRow("3-item list, get first")						<< l <<  1 <<  1 << first;
			QTest::newRow("3-item list, get first, zero start")			<< l <<  0 <<  2 << first;
			QTest::newRow("3-item list, get first, negative start")		<< l << -1 <<  3 << first;

			QTest::newRow("3-item list, get second two")				<< l <<  2 <<  2 << secondTwo;
			QTest::newRow("3-item list, get second two, overrun")		<< l <<  2 << 42 << secondTwo;

			QTest::newRow("3-item list, get third")						<< l <<  3 <<  1 << third;
			QTest::newRow("3-item list, get third, overrun")			<< l <<  3 << 42 << third;
		}
	}
	void testCut()
	{
		QFETCH(VuoList_VuoText, initialList);
		QFETCH(int, startIndex);
		QFETCH(int, itemCount);
		QFETCH(VuoList_VuoText, expectedCutList);

		VuoList_VuoText actualCutList = VuoListCopy_VuoText(initialList);
		VuoListCut_VuoText(actualCutList, startIndex, itemCount);

		verifyEqual(actualCutList, expectedCutList);
	}

	void testRemove_data()
	{
		QTest::addColumn<VuoList_VuoText>("initialList");
		QTest::addColumn<int>("index");
		QTest::addColumn<VuoList_VuoText>("expectedList");

		QTest::newRow("empty list")					<< VuoListCreate_VuoText() <<  0 << VuoListCreate_VuoText();
		QTest::newRow("empty list, staring later")	<< VuoListCreate_VuoText() << 42 << VuoListCreate_VuoText();

		{
			VuoList_VuoText l = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(l, VuoText_make("one"));
			VuoListAppendValue_VuoText(l, VuoText_make("two"));

			VuoList_VuoText missingFirst = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(missingFirst, VuoText_make("two"));

			VuoList_VuoText missingSecond = VuoListCreate_VuoText();
			VuoListAppendValue_VuoText(missingSecond, VuoText_make("one"));

			QTest::newRow("2-item list, remove 0")						<< l <<  0 <<  l;
			QTest::newRow("2-item list, remove 1")						<< l <<  1 <<  missingFirst;
			QTest::newRow("2-item list, remove 2")						<< l <<  2 <<  missingSecond;
			QTest::newRow("2-item list, remove 3")						<< l <<  3 <<  l;
			QTest::newRow("2-item list, remove 42")						<< l << 42 <<  l;
		}
	}
	void testRemove()
	{
		QFETCH(VuoList_VuoText, initialList);
		QFETCH(int, index);
		QFETCH(VuoList_VuoText, expectedList);

		VuoList_VuoText actualList = VuoListCopy_VuoText(initialList);
		VuoListRemoveValue_VuoText(actualList, index);

		verifyEqual(actualList, expectedList);
	}

	void testComparison_data()
	{
		QTest::addColumn<VuoList_VuoText>("a");
		QTest::addColumn<VuoList_VuoText>("b");
		QTest::addColumn<bool>("expectedEqual");
		QTest::addColumn<bool>("expectedALessThanB");

		VuoList_VuoText nullList = NULL;
		VuoList_VuoText emptyList = VuoListCreate_VuoText();

		VuoList_VuoText oneTwo = VuoListCreate_VuoText();
		VuoListAppendValue_VuoText(oneTwo, VuoText_make("one"));
		VuoListAppendValue_VuoText(oneTwo, VuoText_make("two"));

		VuoList_VuoText oneZwei = VuoListCreate_VuoText();
		VuoListAppendValue_VuoText(oneZwei, VuoText_make("one"));
		VuoListAppendValue_VuoText(oneZwei, VuoText_make("zwei"));

		QTest::newRow("null lists")  << nullList  << nullList  << true  << false;
		QTest::newRow("null, empty") << nullList  << emptyList << false << true;
		QTest::newRow("empty, null") << emptyList << nullList  << false << false;
		QTest::newRow("empty lists") << emptyList << emptyList << true  << false;

		QTest::newRow("empty, oneTwo")   << emptyList << oneTwo  << false << true;
		QTest::newRow("null, oneTwo")    << nullList  << oneTwo  << false << true;
		QTest::newRow("oneTwo, oneTwo")  << oneTwo    << oneTwo  << true  << false;
		QTest::newRow("oneTwo, oneZwei") << oneTwo    << oneZwei << false << true;
		QTest::newRow("oneZwei, oneTwo") << oneZwei   << oneTwo  << false << false;
	}
	void testComparison()
	{
		QFETCH(VuoList_VuoText, a);
		QFETCH(VuoList_VuoText, b);
		QFETCH(bool, expectedEqual);
		QFETCH(bool, expectedALessThanB);

		QCOMPARE(VuoList_VuoText_areEqual(a,b), expectedEqual);
		QCOMPARE(VuoList_VuoText_isLessThan(a,b), expectedALessThanB);
	}
};

QTEST_APPLESS_MAIN(TestVuoList)

#include "TestVuoList.moc"
