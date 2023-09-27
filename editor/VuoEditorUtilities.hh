/**
 * @file
 * VuoEditorUtilities interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorWindow;
class VuoNodeClass;
class VuoRecentFileMenu;

/**
 * Miscellaneous shared functions for Vuo Editor.
 */
class VuoEditorUtilities : public QObject
{
	Q_OBJECT

public:
	static QPixmap vuoLogoForDialogs();
	Q_INVOKABLE static QString getHTMLForSVG(QString svgPath, int pointsWide, int pointsHigh);

	static void openUserModulesFolder();
	static void openSystemModulesFolder();

	static bool optionKeyPressedForEvent(QEvent *event);

	static bool isNodeClassEditable(VuoNodeClass *nodeClass, QString &editLabel, QString &sourcePath);
	static bool isNodeClassSuccessorTo(QString oldNodeClass, QString newNodeClass);

	static QList<QMainWindow *> getOpenEditingWindows();
	static QList<VuoEditorWindow *> getOpenCompositionEditingWindows();
	static QList<QMainWindow *> getOpenEditingWindowsStacked();
	static QMainWindow * existingWindowWithFile(const QString &filename);

	static QAction * getRaiseDocumentActionForWindow(QMainWindow *window);
	static VuoRecentFileMenu * getRecentFileMenuForWindow(QMainWindow *window);
	static QMenu * getFileMenuForWindow(QMainWindow *window);
	static void setWindowAsActiveWindow(QMainWindow *window);
	static void setWindowOpacity(QMainWindow *window, int opacity);
};
