/**
 * @file
 * VuoMenu implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoMenu.hh"

/**
 * If the menu is shown with its normal window flags, it never releases focus.
 * Giving it these alternate window flags avoids that.
 */
VuoMenu::VuoMenu(QWidget *parent)
	: QMenu(parent)
{
	setWindowFlags(Qt::Tool|Qt::FramelessWindowHint);
	setFocusPolicy(Qt::ClickFocus);
	setFocus();
}

/**
 * Now that we've set the alternate window flags, the menu doesn't automatically close when an action is performed.
 * Manually close it in that case.
 */
bool VuoMenu::event(QEvent * e)
{
	bool readyToClose = false;

	QKeyEvent *k = dynamic_cast<QKeyEvent *>(e);
	if (e->type() == QEvent::MouseButtonRelease
		|| (e->type() == QEvent::KeyPress && (k->key() == Qt::Key_Return || k->key() == Qt::Key_Enter)))
	{
		QAction *a = activeAction();
		if (a)
			a->activate(QAction::Trigger);

		readyToClose = true;
	}

	if (e->type() == QEvent::FocusOut)
	{
		bool lostFocusToSubmenu = false;

		foreach(QObject *child, children())
		{
			VuoMenu *cm = dynamic_cast<VuoMenu *>(child);
			if (cm && cm->hasFocus())
				lostFocusToSubmenu = true;
		}

		readyToClose = !lostFocusToSubmenu;
	}

	if (readyToClose)
	{
		close();

		VuoMenu *p = dynamic_cast<VuoMenu *>(parent());
		if (p)
			return p->event(e);

		return true;
	}

	return QMenu::event(e);
}
