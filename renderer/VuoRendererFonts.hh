/**
 * @file
 * VuoRendererFonts interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERFONTS_HH
#define VUORENDERERFONTS_HH

/**
 * Provides fonts for rendered items in a composition.
 */
class VuoRendererFonts
{
public:
	static VuoRendererFonts * getSharedFonts(void);

	static double getHorizontalOffset(const QFont font, const QString text);

	static const qreal thickPenWidth;  ///< Width, in pixels at 1:1 zoom, of the top and bottom node edges.  Many other drawing metrics are based on this value.
	static const qreal midPenWidth;  ///< Width, in pixels at 1:1 zoom, of stateful indicator bars and typecast port edges.

	static const qreal nodeTitleFontSize;  ///< Size, in typographic points, of the node's title.
	static const qreal nodeDetailFontSize;  ///< Size, in typographic points, of the node's class name and port labels.
	static const qreal portDetailFontSize;  ///< Size, in typographic points, of port constant flag and typeconverter text.

	QFont nodeTitleFont(void);
	QFont nodeClassFont(void);
	QFont nodePortTitleFont(void);
	QFont nodePortConstantFont(void);
	QFont portPopoverFont(void);

private:
	VuoRendererFonts(void);
	static void addFont(QString font);

	static VuoRendererFonts *sharedFonts;
};

#endif // VUORENDERERFONTS_HH
