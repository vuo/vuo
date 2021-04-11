/**
 * @file
 * VuoPublishedPortDropBox interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A "Publish Port" well that will accept cable drops and publish the internal port
 * connected to the other end of the dragged cable.
 */
class VuoPublishedPortDropBox : public QLabel
{
	Q_OBJECT
public:
	explicit VuoPublishedPortDropBox(QWidget *parent = 0);
	void setHovered(bool hovered);
	void setCurrentlyAcceptingDrops(bool acceptingDrops);

signals:

public slots:

protected:
	void paintEvent(QPaintEvent *event);

private:
	bool isHovered;
	void updateStyleSheet();
	QString getBaseStyleSheet();
};
