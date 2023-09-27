/**
 * @file
 * VuoKeyComboBox implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoKeyComboBox.hh"

#include "VuoHeap.h"

extern "C"
{
	#include "VuoKey.h"
}

/**
 * Helper function for sorting combo box items.
 */
bool VuoKeyComboBox::isPairOfItemsSorted(pair<QString, QVariant> item1, pair<QString, QVariant> item2)
{
	if (item1.first == item2.first)
		return false;

	QString texts[] = { item1.first, item2.first };
	Group groups[2];
	for (int i = 0; i < 2; ++i)
	{
		if (texts[i] == VuoKey_getSummary(VuoKey_Any))
		{
			groups[i] = Any;
		}
		else if (texts[i].length() == 1)
		{
			QRegExp numberCheck("\\d");
			if (numberCheck.exactMatch(texts[i]))
				groups[i] = Numbers;
			else
			{
				QRegExp punctuationCheck("[=\\-\\]\\[';\\\\,/.`]");
				if (punctuationCheck.exactMatch(texts[i]))
					groups[i] = Punctuation;
				else
					groups[i] = Letters;
			}
		}
		else
		{
			if (texts[i].endsWith("Arrow"))
				groups[i] = Arrows;
			else if (texts[i].startsWith("Keypad"))
				groups[i] = Keypad;
			else
			{
				QRegExp fKeyCheck("F\\d\\d?");
				if (fKeyCheck.exactMatch(texts[i]))
					groups[i] = FKeys;
				else
					groups[i] = OtherKeys;
			}
		}
	}

	if (groups[0] != groups[1])
		return groups[0] < groups[1];

	if (groups[0] == Numbers)
	{
		int n1 = texts[0].toInt();
		int n2 = texts[1].toInt();
		if (n1 == 0)
			return false;
		if (n2 == 0)
			return true;
		return n1 < n2;
	}

	if (groups[0] == FKeys)
	{
		int n1 = texts[0].mid(1).toInt();
		int n2 = texts[1].mid(1).toInt();
		return n1 < n2;
	}

	return texts[0] < texts[1];
}

/**
 * Creates a combo box populated with a list of all VuoKey values.
 */
VuoKeyComboBox::VuoKeyComboBox(QWidget *parent):
	VuoComboBox(parent)
{
	view()->installEventFilter(this);

	vector<pair<QString, QVariant>> itemTextAndData;
	for (int i = VuoKey_Any; i <= VuoKey_Kana; ++i)
	{
		VuoKey key               = (VuoKey)i;
		char *summary            = VuoKey_getSummary(key);
		QString summaryAsUnicode = QString::fromUtf8(summary);
		itemTextAndData.push_back(make_pair(summaryAsUnicode, key));
		free(summary);
	}

	sort(itemTextAndData.begin(), itemTextAndData.end(), isPairOfItemsSorted);

	for (vector<pair<QString, QVariant>>::iterator i = itemTextAndData.begin(); i != itemTextAndData.end(); ++i)
	{
		addItem(i->first, i->second);
	}
}

/**
 * Selects the given VuoKey in the combo box.
 */
void VuoKeyComboBox::setCurrentKey(VuoKey key)
{
	int index = findData(key);
	if (index >= 0)
		setCurrentIndex(index);
}

/**
 * Returns the currently selected VuoKey.
 */
VuoKey VuoKeyComboBox::getCurrentKey(void)
{
	QVariant currentKeyAsData = itemData(currentIndex());
	return (VuoKey)currentKeyAsData.toInt();
}

/**
 * Captures all keypresses, and uses them to change the VuoKey selection.
 */
void VuoKeyComboBox::keyPressEvent(QKeyEvent *e)
{
	if ((e->nativeVirtualKey() == 0 && e->text().isEmpty()) || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Escape)
	{
		QComboBox::keyPressEvent(e);
		return;
	}

	VuoKey key = VuoKey_makeFromMacVirtualKeyCode(e->nativeVirtualKey());
	setCurrentKey(key);
}

/**
 * Captures all keypresses that would normally go to the child widget (list popup),
 * and uses them to change the VuoKey selection.
 */
bool VuoKeyComboBox::eventFilter(QObject *object, QEvent *e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
		keyPressEvent(keyEvent);
		QModelIndex index = model()->index(currentIndex(), 0);
		((QAbstractItemView *)object)->setCurrentIndex(index);
		return true;
	}
	return false;
}
