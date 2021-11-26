/**
 * @file
 * VuoRendererCommon interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;

/**
 * Shared Qt utilities.
 */
class VuoRendererCommon
{
public:
	static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
	static QString externalizeVuoNodeLinks(VuoCompiler *compiler, QString markdownText, bool outputAbsoluteLinks);
	static QString getStyleSheet(bool isDark);
};
