/**
 * @file
 * VuoMenu interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOMENU_HH
#define VUOMENU_HH

/**
 * Disgusting hack.
 * Necessary to work around a bug in Qt 5, wherein QMenu context menus indefinitely retain focus,
 * preventing the window's QMenu from reappearing after the context menu has been dismissed.
 * @todo Revert once Qt fixes this bug.
 * https://b33p.net/kosada/node/5130
 * https://b33p.net/kosada/node/5096
 */
class VuoMenu : public QMenu
{
	Q_OBJECT
public:
	VuoMenu(QWidget *parent = 0);

private:
	bool event(QEvent * e);
};

#endif // VUOMENU_HH
