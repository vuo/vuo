/**
 * @file
 * VuoPublishedPortSidebar interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCable;
class VuoEditorComposition;
class VuoPort;
class VuoPublishedPort;
class VuoRendererCable;
class VuoRendererPort;
class VuoRendererPublishedPort;

namespace Ui
{
	class VuoPublishedPortSidebar;
}

/**
 * A docked window displaying the list of published input and/or output ports associated with the input @c composition.
 */
class VuoPublishedPortSidebar : public QDockWidget
{
	Q_OBJECT
public:
	explicit VuoPublishedPortSidebar(QWidget *parent, VuoEditorComposition *composition, bool isInput, bool enableProtocolChanges=true);
	void updateActiveProtocol();
	void concludePublishedCableDrag(QMouseEvent *event, VuoCable *cableInProgress, bool cableInProgressWasNew);
	QMenu * getProtocolsContextMenu();
	QAction * getRemoveProtocolAction();
	VuoRendererPublishedPort * getPublishedPortUnderCursorForEvent(QMouseEvent *event, qreal tolerance=0, bool limitPortCollisionRange=false);
	QPoint getGlobalPosOfPublishedPort(VuoRendererPublishedPort *port);
	bool getMenuSelectionInProgress();
	static QColor getActiveProtocolPortColor(int protocolIndex, bool isInput);
	string showPublishedPortNameEditor(VuoRendererPublishedPort *port);
	void limitAllowedPortTypes(const set<string> &allowedPortTypes);

signals:
	void newPublishedPortRequested(string typeName, bool isInput);  ///< Emitted when a new externally visible published port is to be added.
	void publishedPortNameEditorRequested(VuoRendererPublishedPort *port, bool useUndoStack);  ///< Emitted in order to display a published port name editor.
	void publishedPortDetailsChangeRequested(VuoRendererPublishedPort *port, json_object *newDetails); ///< Emitted when a published port's details have been changed in a details editor.
	void inputEditorRequested(VuoRendererPort *port); ///< Emitted in order to display a published port input editor.
	void externalPortUnpublicationRequested(VuoRendererPublishedPort *port);  ///< Emitted when an externally visible published port is to be unpublished.
	void portPublicationRequestedViaDropBox(VuoPort *port, bool forceEventOnlyPublication, bool useUndoStackMacro);  ///< Emitted when an internal port is to be published.
	void portPublicationRequestedViaSidebarPort(VuoPort *internalPort, VuoPublishedPort *externalPort, bool forceEventOnlyPublication, VuoPort *portToSpecialize, string specializedTypeName, string typecastToInsert, bool useUndoStackMacro);  ///< Emitted when an internal port is to be published in association with a specific external published port.
	void componentsRemoved(QList<QGraphicsItem *> removedComponents, string commandDescription="Remove"); ///< Emitted when components are to be removed.
	void publishedPortPositionsUpdated();  ///< Emitted when the stored positions of the published ports within the sidebars have been updated.
	void publishedPortsReordered(vector<VuoPublishedPort *> publishedPorts, bool isInput);  ///< Emitted when published ports are reordered by the user.
	void undoStackMacroBeginRequested(QString commandName); ///< Emitted when the upcoming sequence of requested operations should be coalesced in an Undo stack macro.
	void undoStackMacroEndRequested(); ///< Emitted when the sequence of operations to be coalesced into an Undo stack macro has completed.
	void closed(); ///< Emitted when the published port sidebar is closed.

public slots:
	void updatePortList();
	void highlightEligibleDropLocations(VuoRendererPort *internalFixedPort, bool eventOnlyConnection);
	void clearEligibleDropLocationHighlighting();
	void externalMoveEvent();
	void updateHoverHighlighting(QMouseEvent *event, qreal tolerance=0);
	void clearHoverHighlighting();

private slots:
	void populatePortTypeMenus();
	void newPublishedPortTypeSelected();
	void updateColor(bool isDark);

protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	Ui::VuoPublishedPortSidebar *ui;
	VuoEditorComposition *composition;

	QMenu *contextMenuPortOptions;
	QMenu *menuChangeProtocol;
	QMenu *menuAddPort;

	QMenu *contextMenuRemoveProtocol;
	QAction *contextMenuActionRemoveProtocol;

	bool isInput;
	bool portTypeMenusPopulated;
	set<string> allowedPortTypes;

	bool canListPublishedPortAliasFor(VuoRendererPort *port);
	bool isPublishedPortDropBoxUnderCursorForEvent(QMouseEvent *event);
	bool isActiveProtocolLabelUnderCursorForEvent(QContextMenuEvent *event);
	void appendPublishedPortToList(VuoPublishedPort *port, bool select);
	static QColor getActiveProtocolHeadingColor(int protocolIndex, bool isInput);
	void showPublishedPortDetailsEditor(VuoRendererPublishedPort *port);

#ifdef VUO_PRO
#include "pro/VuoPublishedPortSidebar_Pro.hh"
#endif
};

