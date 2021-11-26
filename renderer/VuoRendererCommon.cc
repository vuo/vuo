/**
 * @file
 * VuoRendererCommon implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
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

	if (context.category && strcmp(context.category, "qt.network.ssl") == 0
	 && message == "Error receiving trust for a CA certificate")
		// Quell this warning; a Qt developer says:
		// "you can ignore this message … it's a result of some certificates removed on OS X 10.11."
		// https://bugreports.qt.io/browse/QTBUG-54161
		return;

	if (type == QtWarningMsg && message.startsWith("QVariant::save: unable to save type 'VuoRendererPublishedPort*' (type id:"))
		// Quell this warning; VuoRendererPorts only exist within a single editor process lifetime; we don't need to serialize them.
		return;

	if (type == QtWarningMsg && message.startsWith("Layer-backing can not be explicitly controlled"))
		// Quell this warning; `QT_MAC_WANTS_LAYER` does still make a difference.
		// https://b33p.net/kosada/vuo/vuo/-/issues/17855
		// https://b33p.net/kosada/vuo/vuo/-/issues/13819
		// https://bugreports.qt.io/browse/QTBUG-81370
		return;

	if (type == QtWarningMsg && message.startsWith("Display non non-main thread! Deferring to main thread"))
		// Quell this warning; it's internal to Qt.
		return;

	VuoLog(VuoLog_moduleName,
		   context.file ? context.file : (context.category ? context.category : "?"),
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

/**
 * Returns the app-wide stylesheet.
 */
QString VuoRendererCommon::getStyleSheet(bool isDark)
{
	QFile f(":/Vuo.qss");
	f.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&f);
	QString styles = ts.readAll();

	if (isDark)
	{
		QFile fDark(":/pro/VuoDark.qss");
		fDark.open(QFile::ReadOnly | QFile::Text);
		QTextStream tsDark(&fDark);
		styles += tsDark.readAll();
	}

	return styles;
}
