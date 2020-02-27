/**
 * @file
 * VuoKeyComboBox interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoComboBox.hh"

extern "C"
{
#include "VuoKey.h"
}

/**
 * A combo box that displays a list of VuoKeys and reacts to keypresses by selecting the corresponding VuoKey.
 */
class VuoKeyComboBox : public VuoComboBox
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

	static bool isPairOfItemsSorted(pair<QString, QVariant> item1, pair<QString, QVariant> item2);

public:
	VuoKeyComboBox(QWidget *parent);
	void setCurrentKey(VuoKey key);
	VuoKey getCurrentKey(void);

public slots:
	void keyPressEvent(QKeyEvent *e);

public:
	bool eventFilter(QObject *object, QEvent *e);
};
