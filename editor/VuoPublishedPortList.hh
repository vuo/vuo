/**
 * @file
 * VuoPublishedPortList interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoEditorComposition;
class VuoPublishedPort;
class VuoRendererPort;
class VuoRendererPublishedPort;

/**
 * A widget displaying published input and/or output ports.
 */
class VuoPublishedPortList : public QListWidget
{
	Q_OBJECT
public:
	explicit VuoPublishedPortList(QWidget *parent = 0);
	bool getInput();
	void setInput(bool isInput);
	void setComposition(VuoEditorComposition *composition);
	QSize sizeHint() const;
	VuoRendererPublishedPort * getPublishedPortAtGlobalPos(QPoint globalPos, qreal xTolerance=0, bool limitPortCollisionRange=false);
	QPoint getGlobalPosOfPublishedPort(VuoRendererPublishedPort *port);
	void clearSelection();
	void setFillVerticalSpace(bool fill);
	void adoptDropEvent(QDropEvent *event);
	bool getMenuSelectionInProgress();

	static const int publishedPortPointerIndex; ///< A @c VuoRendererPublishedPort instance.
	static const qreal componentCollisionRange; ///< The search range used in locating ports near the cursor.

signals:
	void publishedPortModified();  ///< Emitted when any published port within this list has been unpublished or renamed.
	void publishedPortNameEditorRequested(VuoRendererPublishedPort *port, bool useUndoStack);  ///< Emitted in order to display a published port name editor.
	void publishedPortDetailsEditorRequested(VuoRendererPublishedPort *port);  ///< Emitted in order to display a published port details editor.
	void inputEditorRequested(VuoRendererPort *port); ///< Emitted in order to display a published port input editor.
	void externalPortUnpublicationRequested(VuoRendererPublishedPort *port);  ///< Emitted when an externally visible published port is to be unpublished.
	void publishedPortPositionsUpdated();  ///< Emitted when the stored positions of the published ports within the sidebars have been updated.
	void mouseMoveEventReceived(QMouseEvent *event, qreal hoverTolerance);  ///< Emitted when this published port list has received a mouseMove event.
	void publishedPortsReordered(vector<VuoPublishedPort *> publishedPorts, bool isInput);  ///< Emitted when published ports are reordered by the user.

public slots:
	void updatePublishedPortLocs();

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private slots:
	void editPublishedPortValue();
	void editPublishedPortDetails();
	void renamePublishedPort();
	void renamePublishedPort(QListWidgetItem *portListItem);
	void unpublishExternalPort();
	void updatePortOrder();

private:
	bool valueEditableForPublishedPort(VuoPublishedPort *port);
	bool detailsEditableForPublishedPort(VuoPublishedPort *port);
	void editPublishedPortValue(VuoRendererPublishedPort *port);
	bool isInput;
	VuoEditorComposition *composition;
	QAction *contextMenuSetPortConstant;
	QAction *contextMenuSetPortDetails;
	QAction *contextMenuRenamePublishedPort;
	QAction *contextMenuUnpublishPort;
	bool fillVerticalSpace;
	bool forwardingEventsToCanvas;
	bool menuSelectionInProgress;
};
