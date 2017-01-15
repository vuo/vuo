/**
 * @file
 * VuoKeyComboBox implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

extern "C"
{
	#include "VuoKey.h"
}

/**
 * A combo box that displays a list of VuoKeys and reacts to keypresses by selecting the corresponding VuoKey.
 */
class VuoKeyComboBox : public QComboBox
{
	Q_OBJECT

private:
	/**
	 * Groups for initial sorting of combo box items.
	 */
	enum Group
	{
		Any,
		Letters,
		Numbers,
		Punctuation,
		Arrows,
		Keypad,
		FKeys,
		OtherKeys
	};

	/**
	 * Helper function for sorting combo box items.
	 */
	static bool isPairOfItemsSorted(pair<QString, QVariant> item1, pair<QString, QVariant> item2)
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

public:
	/**
	 * Creates a combo box populated with a list of all VuoKey values.
	 */
	VuoKeyComboBox(QWidget *parent)
		: QComboBox(parent)
	{
		view()->installEventFilter(this);

		vector< pair<QString, QVariant> > itemTextAndData;
		for (VuoKey key = VuoKey_Any; key <= VuoKey_Kana; ++key)
		{
			char *summary = VuoKey_getSummary(key);
			QString summaryAsUnicode = QString::fromUtf8(summary);
			itemTextAndData.push_back( make_pair(summaryAsUnicode, key) );
			free(summary);
		}

		sort(itemTextAndData.begin(), itemTextAndData.end(), isPairOfItemsSorted);

		for (vector< pair<QString, QVariant> >::iterator i = itemTextAndData.begin(); i != itemTextAndData.end(); ++i)
		{
			addItem(i->first, i->second);
		}
	}

	/**
	 * Selects the given VuoKey in the combo box.
	 */
	void setCurrentKey(VuoKey key)
	{
		int index = findData(key);
		if (index >= 0)
			setCurrentIndex(index);
	}

	/**
	 * Returns the currently selected VuoKey.
	 */
	VuoKey getCurrentKey(void)
	{
		QVariant currentKeyAsData = itemData( currentIndex() );
		return (VuoKey)currentKeyAsData.toInt();
	}

public slots:
	/**
	 * Captures all keypresses, and uses them to change the VuoKey selection.
	 */
	void keyPressEvent(QKeyEvent *e)
	{
		if ((e->nativeVirtualKey() == 0 && e->text().isEmpty()) ||
				e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Escape)
		{
			QComboBox::keyPressEvent(e);
			return;
		}

		VuoKey key = VuoKey_makeFromMacVirtualKeyCode( e->nativeVirtualKey() );
		setCurrentKey(key);
	}

public:
	/**
	 * Captures all keypresses that would normally go to the child widget (list popup),
	 * and uses them to change the VuoKey selection.
	 */
	bool eventFilter(QObject *object, QEvent *e)
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
};
