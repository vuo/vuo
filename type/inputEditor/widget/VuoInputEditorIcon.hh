/**
 * @file
 * VuoInputEditorIcon interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORICON_HH
#define VUOINPUTEDITORICON_HH

#include <QtGui/QIcon>

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
		qreal devicePixelRatio = qApp->devicePixelRatio();
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
		QIcon *icon = new QIcon(*px);
		delete px;

		return icon;
	}
};

#endif
