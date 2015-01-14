/**
 * @file
 * TestVuoList implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

extern "C" {
#include "TestVuoTypes.h"
#include "VuoText.h"
#include "VuoList_VuoText.h"
}

/**
 * Tests the VuoList_VuoText type (no need to test other list types, since they're all automatically generated from the same source).
 */
class TestVuoList : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		VuoHeap_init();
	}

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

		QCOMPARE(QString(VuoListGetValueAtIndex_VuoText(l,1)), QString("zero"));
		QCOMPARE(QString(VuoListGetValueAtIndex_VuoText(l,2)), QString("one"));
		QCOMPARE(QString(VuoListGetValueAtIndex_VuoText(l,3)), QString("two"));

		{
			char *lSerialized = VuoList_VuoText_stringFromValue(l);
			VuoList_VuoText lUnserialized = VuoList_VuoText_valueFromString(lSerialized);
			QCOMPARE(VuoListGetCount_VuoText(l), VuoListGetCount_VuoText(lUnserialized));
			unsigned long itemCount = VuoListGetCount_VuoText(l);
			for (unsigned long i = 1; i <= itemCount; ++i)
			{
				VuoText lItem = VuoListGetValueAtIndex_VuoText(l, i);
				VuoText lUnserializedItem = VuoListGetValueAtIndex_VuoText(lUnserialized, i);
				QCOMPARE(QString(lItem), QString(lUnserializedItem));
			}
			free(lSerialized);
		}
	}
};

QTEST_APPLESS_MAIN(TestVuoList)

#include "TestVuoList.moc"
