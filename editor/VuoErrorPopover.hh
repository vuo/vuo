/**
 * @file
 * VuoErrorPopover interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoPopover.hh"
class VuoCompilerIssue;

/**
 * A popover for displaying an error message within a composition.
 */
class VuoErrorPopover : public VuoPopover
{
	Q_OBJECT
public:
	explicit VuoErrorPopover(VuoCompilerIssue &issue, QWidget *parent);

public slots:
	void setWindowLevelAndVisibility(bool top);
	void setWindowLevel(bool top);

protected:
	void paintEvent(QPaintEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);

private slots:
	void helpButtonClicked();
	void updateColor(bool isDark);

private:
	void setStyle();
	void setArrowSide(Qt::AnchorPoint arrowSide);

	QString helpPath;
	QLabel *textLabel;
	QVBoxLayout *layout;
	Qt::AnchorPoint arrowSide;
	int arrowTipY;

	// User interaction
	bool dragInProgress;
	QPoint positionBeforeDrag;
};
