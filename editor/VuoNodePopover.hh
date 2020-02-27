/**
 * @file
 * VuoNodePopover interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoPanelDocumentation.hh"

class VuoNode;
class VuoNodeClass;
class VuoNodeSet;
class VuoCompiler;
class VuoRendererNode;

/**
 * A popover widget for displaying information about a node or node class.
 */
class VuoNodePopover : public VuoPanelDocumentation
{
	Q_OBJECT
public:
	explicit VuoNodePopover(VuoNodeClass *nodeClass, VuoCompiler *compiler, QWidget *parent=0);
	void cleanup();
	~VuoNodePopover();
	int getTextWidth();
	void prepareResourcesAndShow();
	bool containsImage();
	VuoNodeClass * getNodeClass();
	VuoRendererNode * getModelNode();
	QString getSelectedText();

	static QString generateTextStyleString(void);

protected:
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

public slots:
	void setTextWidth(int width);

private slots:
	void updateColor(bool isDark);

signals:
	void popoverDisplayRequested(QWidget *, QString); ///< Emitted when a node popover is to be displayed.

private:
	static const int defaultPopoverTextWidth;
	static const int margin;
	QString generateNodePopoverTextHeader();
	QString generateNodePopoverText(bool isDark);
	QString generateNodeClassDescription(string smallTextColor);
	VuoNodeClass *nodeClass;
	VuoNodeSet *nodeSet;
	VuoNode *node;
	QLabel *headerLabel;
	QLabel *textLabel;
	bool displayModelNode;
	VuoRendererNode *modelNode;
	QGraphicsView *modelNodeView;
	QVBoxLayout *layout;
	VuoCompiler *compiler;
	bool popoverHasBeenShown;

	void initialize();

	// Style
	void setStyle();

#if VUO_PRO
#include "pro/VuoNodePopover_Pro.hh"
#endif
};

