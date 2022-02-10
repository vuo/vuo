/**
 * @file
 * VuoInputEditorSession interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorComposition;
class VuoEditorWindow;
class VuoInputEditorManager;
class VuoPublishedPortSidebar;
class VuoRendererPort;

#include "VuoHeap.h"

/**
 * Displays an input editor for a port, along with any subsequent input editors opened by tabbing.
 * Keeps track of the changed input values.
 */
class VuoInputEditorSession : public QObject
{
	Q_OBJECT

public:
	VuoInputEditorSession(VuoInputEditorManager *inputEditorManager, VuoEditorComposition *composition, VuoPublishedPortSidebar *sidebar, QMainWindow *window);
	map<VuoRendererPort *, pair<string, string> > execute(VuoRendererPort *port, bool forwardTabTraversal);

protected:
	bool eventFilter(QObject *object, QEvent *event) VuoWarnUnusedResult;

private slots:
	void updateValueForEditedPort(json_object *newValue);

private:
	void showInputEditor(VuoRendererPort *port, bool forwardTabTraversal);
	void showPreviousInputEditor();
	void showNextInputEditor();

	VuoInputEditorManager *inputEditorManager;
	VuoEditorComposition *composition;
	VuoPublishedPortSidebar *sidebar;
	QMainWindow *window;
	VuoRendererPort *portWithOpenInputEditor;
	QPoint firstPortLeftCenterGlobal;
	map<VuoRendererPort *, string> startValueForPort;
	map<VuoRendererPort *, string> finalValueForPort;
};
