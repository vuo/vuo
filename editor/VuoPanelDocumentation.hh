/**
 * @file
 * VuoPanelDocumentation interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoHeap.h"

/**
 * A widget for displaying content within the documentation panel of the node library.
 */
class VuoPanelDocumentation : public QWidget
{
	Q_OBJECT
public:
	explicit VuoPanelDocumentation(QWidget *parent = 0);
	virtual QString getSelectedText();
	
signals:
	void textSelectionChanged(); ///< Emitted when the documentation's text selection changes.

protected slots:
	void linkHovered(const QString &link);

protected:
	bool event(QEvent *event) VuoWarnUnusedResult;
	QString getDescriptionForURL(const QString &url);
	void hideEvent(QHideEvent *event);

private:
	bool documentationEntered; ///< Whether the cursor is currently positioned over this widget.
	QTimer *tooltipTimer;
};
