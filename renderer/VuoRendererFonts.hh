/**
 * @file
 * VuoRendererFonts interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Provides fonts for rendered items in a composition.
 */
class VuoRendererFonts
{
public:
	static VuoRendererFonts * getSharedFonts(void);

	static double getHorizontalOffset(const QFont font, const QString text);

	static const qreal thickPenWidth;  ///< Width, in pixels at 1:1 zoom, of the top and bottom node edges.  Many other drawing metrics are based on this value.
	static const qreal midPenWidth;  ///< Width, in pixels at 1:1 zoom, of typecast port edges.

	static const QString fontFamily;       ///< The font family used to render all text in Vuo.

	static const qreal nodeTitleFontSize;  ///< Size, in typographic points, of the node's title.
	static const qreal nodeClassFontSize;  ///< Size, in typographic points, of the node's class name.
	static const qreal portTitleFontSize;  ///< Size, in typographic points, of the node's port labels.
	static const qreal portDetailFontSize;  ///< Size, in typographic points, of port constant flag and typeconverter text.

	QFont nodeTitleFont(void);
	QFont nodeClassFont(void);
	QFont nodePortTitleFont(void);
	QFont nodePortConstantFont(void);
	QFont portPopoverFont(void);
	QFont dialogHeadingFont();
	QFont dialogBodyFont();
	QFont commentFont(void);

	QString getCSS(QFont font);

private:
	VuoRendererFonts(void);
	static void addFont(QString font);

	static VuoRendererFonts *sharedFonts;
};

