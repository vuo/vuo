/**
 * @file
 * VuoRendererCommon implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRendererCommon.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoNodeClass.hh"
#include "VuoNodeSet.hh"

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

/**
 * Maps `vuo-node://` and `vuo-nodeset://` links within the provided `markdownText` to
 * the appropriate links for use in documentation outside Vuo editor.
 *
 * If `outputAbsoluteLinks` is true, this function emits links like `https://doc.vuo.org/2.0.0/node/vuo.serial/vuo.serial.configure.html`.
 * If false, emits links relative to the node set folder, e.g., `../vuo.serial/vuo.serial.configure.html`.
 */
QString VuoRendererCommon::externalizeVuoNodeLinks(VuoCompiler *compiler, QString markdownText, bool outputAbsoluteLinks)
{
	QString filteredText(markdownText);
	QRegularExpression vuoNodeLink("\\[(.*)\\](\\(vuo-node://(.*)\\))", QRegularExpression::InvertedGreedinessOption);
	QRegularExpression vuoNodeSetLink("\\[(.*)\\](\\(vuo-nodeset://(.*)\\))", QRegularExpression::InvertedGreedinessOption);

	QString prefix = "../";
	if (outputAbsoluteLinks)
		prefix = "https://doc.vuo.org/" VUO_VERSION_STRING "/node/";

	// Map node class links.
	size_t startPos = 0;
	QRegularExpressionMatch match = vuoNodeLink.match(filteredText, startPos);
	while (match.hasMatch())
	{
		QString nodeClassDisplayTitle = match.captured(1);
		QString nodeClassName = match.captured(3);

		VuoCompilerNodeClass *nodeClass = compiler->getNodeClass(nodeClassName.toStdString());
		QString nodeSetName = (nodeClass? QString::fromStdString(nodeClass->getBase()->getNodeSet()->getName()) : "");
		QString mappedLink = QString("[")
								 .append(nodeClassDisplayTitle)
								 .append("](")
								 .append(prefix)
								 .append(nodeSetName)
								 .append("/")
								 .append(nodeClassName)
								 .append(".html)");

		filteredText.replace(match.capturedStart(), match.capturedLength(), mappedLink);
		startPos = (match.capturedStart() + mappedLink.length());
		match = vuoNodeLink.match(filteredText, startPos);
	}

	// Map node set links.
	startPos = 0;
	match = vuoNodeSetLink.match(filteredText, startPos);
	while (match.hasMatch())
	{
		QString nodeSetDisplayTitle = match.captured(1);
		QString nodeSetName = match.captured(3);

		QString mappedLink = QString("[")
								 .append(nodeSetDisplayTitle)
								 .append("](")
								 .append(prefix)
								 .append(nodeSetName)
								 .append("/index.html)");

		filteredText.replace(match.capturedStart(), match.capturedLength(), mappedLink);
		startPos = (match.capturedStart() + mappedLink.length());
		match = vuoNodeSetLink.match(filteredText, startPos);
	}

	return filteredText;
}
