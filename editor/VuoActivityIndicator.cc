/**
 * @file
 * VuoActivityIndicator implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoActivityIndicator.hh"

/**
 * Creates an activity indicator.
 *
 * To animate the activity indicator, periodically call QAction::setIcon() with icons created
 * from this class, increasing @c ticks by 1 each time. The first time, @c ticks should be 0.
 */
VuoActivityIndicator::VuoActivityIndicator(unsigned int ticks)
{
	this->ticks = ticks;
}

/**
 * Overrides QIconEngine::pixmap().
 */
QPixmap VuoActivityIndicator::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
	QPixmap pixmap(size);

	// Allow paint() to set a transparent background.
	QBitmap bitmap(size);
	bitmap.clear();
	pixmap.setMask(bitmap);

	QPainter painter(&pixmap);
	paint(&painter, QRect(QPoint(0,0),size), mode, state);
	return pixmap;
}

/**
 * Draws the activity indicator.
 *
 * Overrides QIconEngine::paint().
 */
void VuoActivityIndicator::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
	painter->setRenderHint(QPainter::Antialiasing, true);

	painter->setBackground(Qt::transparent);
	painter->eraseRect(rect);

	float outerRadius = 0.7f * std::min(rect.width(), rect.height()) / 2;
	float innerRadius = 0.7f * outerRadius;
	float penWidth = 0.3f * innerRadius;
	int numLines = 12;

	for (int i = 0; i < numLines; ++i)
	{
		int alpha = (i + 1) / (float)numLines * 255.0f;
		QPen pen(QColor(0, 0, 0, alpha), penWidth);
		painter->setPen(pen);

		float radiansForSpoke = (i + ticks) / (float)numLines * 2.0f * M_PI;
		QPointF innerRectCenter(innerRadius * cos(radiansForSpoke), innerRadius * sin(radiansForSpoke));
		QPointF outerRectCenter(outerRadius * cos(radiansForSpoke), outerRadius * sin(radiansForSpoke));
		painter->drawLine(rect.center() + innerRectCenter, rect.center() + outerRectCenter);
	}
}

/**
 * Implements pure virtual function QIconEngine::clone().
 */
QIconEngine * VuoActivityIndicator::clone() const
{
	return NULL;
}
