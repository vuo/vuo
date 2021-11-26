/**
 * @file
 * VuoEditorConsoleWindow interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoConsole;
class VuoConsoleToolbar;
class VuoRecentFileMenu;

/**
 * Helper class for VuoConsole.
 */
class VuoConsoleWindow : public QMainWindow {
	Q_OBJECT

public:
	VuoConsoleWindow(VuoConsole *console, QWidget *screenMate);

	void setModel(const QStringList &logs);
	QStringList getModel(void);
	QList<QVariant> getSelectedIndices(void);

	VuoRecentFileMenu * getRecentFileMenu(void);

private:
	void populateMenus(VuoConsole *console);
	void updateColor(void);
	void updateOpacity(void);

	QQuickWidget *quickContainer;
	VuoConsoleToolbar *toolbar;
	VuoRecentFileMenu *recentFileMenu;
};
