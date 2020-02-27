/**
 * @file
 * VuoActivityIndicator interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A rendering engine for a circular activity indicator, similar in style to the native Mac/iOS activity indicator.
 */
class VuoActivityIndicator : public QIconEngine
{
private:
	unsigned int ticks;

public:
	VuoActivityIndicator(unsigned int ticks);
	QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);
	void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state);
	QIconEngine * clone() const;
};
