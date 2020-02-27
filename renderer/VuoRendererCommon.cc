/**
 * @file
 * VuoRendererCommon implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererCommon.hh"

/**
 * QtMessageHandler that routes Qt messages through VuoLog, so they appear in crash reports.
 */
void VuoRendererCommon::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
	const char *func = context.function;
	if (!func)
	{
		if (type == QtDebugMsg)
			func = "QMessageLogger::debug";
		else if (type == QtInfoMsg)
			func = "QMessageLogger::info";
		else if (type == QtWarningMsg)
			func = "QMessageLogger::warning";
		else if (type == QtCriticalMsg)
			func = "QMessageLogger::critical";
		else if (type == QtFatalMsg)
			func = "QMessageLogger::fatal";
		else
			func = "?";
	}

	VuoLog(context.file ? context.file : (context.category ? context.category : "?"),
		   context.line,
		   func,
		   "%s", message.toUtf8().constData());
}
