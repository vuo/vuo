/**
 * @file
 * VuoNodeClassList implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompilerNodeClass;
class VuoEditorComposition;
class VuoNodeClass;
class VuoRendererNode;

/**
 * A widget allowing node classes to be dragged out of it (and onto, e.g., a composition editor canvas).
 */
class VuoNodeClassList : public QListWidget
{
	Q_OBJECT

public:
	explicit VuoNodeClassList(QWidget *parent=0);
	~VuoNodeClassList();
	void setFilterText(QString filterText);
	void enablePopovers();
	void enablePopoversAndDisplayCurrent();
	VuoCompilerNodeClass * getNodeClassForItem(QListWidgetItem *item);
	void disablePopovers();
	bool selectionFinalized;  ///< True when list filtering has completed.
	static void populateContextMenuForNodeClass(QMenu *contextMenu, VuoCompilerNodeClass *nodeClass);

signals:
	void componentsAdded(QList<QGraphicsItem *> addedComponents, VuoEditorComposition *target); ///< Emitted when the user has used the Node Library to add nodes to the composition.
	void nodePopoverRequestedForClass(VuoNodeClass *nodeClass); ///< Emitted when documentation for the specified node class has been requested.
	void nodeClassListHeightChanged(int newHeight); ///< Emitted when the node class list's height changes.

protected slots:
	void addDoubleClickedNode(QListWidgetItem *nodeClass);
	void displayPopoverForItem(QListWidgetItem *targetNodeClassItem);

protected:
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent * event);
	void contextMenuEvent(QContextMenuEvent * event);

private:
	void startDrag(Qt::DropActions supportedActions);
	VuoRendererNode * createSelectedNode(QListWidgetItem *nodeClass, string title="", double x=0, double y=0);
	QListWidgetItem * getItemAtGlobalPos(QPoint globalPos);
	bool popoversEnabled;
};
