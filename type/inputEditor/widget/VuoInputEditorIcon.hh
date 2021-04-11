/**
 * @file
 * VuoInputEditorIcon interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <QtGui/QIcon>

#include "VuoInputEditor.hh"

/**
 * Provides functions for rendering icons.
 */
class VuoInputEditorIcon
{
public:
	/**
	 * Renders the specified @c render block onto a pixmap of the given point size, respecting the device pixel ratio.
	 */
	static QPixmap * renderPixmap(void (^render)(QPainter &p), int width=16, int height=16)
	{
		if (!render)
			return NULL;

		qreal devicePixelRatio = qApp->primaryScreen()->devicePixelRatio();
		QPixmap *px = new QPixmap(width*devicePixelRatio, height*devicePixelRatio);
		px->setDevicePixelRatio(devicePixelRatio);
		px->fill(Qt::transparent);

		QPainter p(px);
		p.setRenderHint(QPainter::Antialiasing);
		render(p);

		return px;
	}

	/**
	 * Renders the specified @c render block onto an icon of the given point size, respecting the device pixel ratio.
	 */
	static QIcon * renderIcon(void (^render)(QPainter &p), int width=16, int height=16)
	{
		QPixmap *px = renderPixmap(render, width, height);

		// https://b33p.net/kosada/node/12462
		px->setDevicePixelRatio(1);

		QIcon *icon = new QIcon(*px);
		delete px;

		return icon;
	}

	/**
	 * Returns the icon for an error within a line edit.
	 */
	static QPixmap *renderErrorPixmap(bool isDark)
	{
		const int size = 12;
		return renderPixmap(^(QPainter &p){
			p.setPen(QPen(QColor(isDark ? "#a82c2c" : "#dc3e39"), 0));
			p.setBrush(QColor(isDark ? "#cc433f" : "#ff554e"));
			QRectF r(0.5, 0.5, size-1, size-1);
			p.drawEllipse(r);
			p.setPen(QColor(255,255,255, isDark ? 192 : 255));
			p.setBrush(Qt::NoBrush);
			p.setFont(QFont(VuoInputEditor::getDefaultFont().family(), 11, QFont::Bold));
			p.drawText(r, Qt::AlignCenter, "!");
		},
		size, size);
	}
};
