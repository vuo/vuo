/**
 * @file
 * VuoEditorUtilities implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoEditorUtilities.hh"

#include "VuoCodeWindow.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoEditorWindow.hh"
#include "VuoFileUtilities.hh"
#include "VuoNodeClass.hh"

#ifdef __APPLE__
#include <objc/objc-runtime.h>
#include <ApplicationServices/ApplicationServices.h> // for kCGEventSourceStateCombinedSessionState
#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#endif

/**
 * Returns a retina-compatible logo of the appropriate size for popup dialog windows.
 */
QPixmap VuoEditorUtilities::vuoLogoForDialogs()
{
	qreal devicePixelRatio = qApp->primaryScreen()->devicePixelRatio();
	QPixmap logo(QStringLiteral(":/Icons/vuo.png"));
	logo = logo.scaledToHeight(64 * devicePixelRatio, Qt::SmoothTransformation);
	logo.setDevicePixelRatio(devicePixelRatio);
	return logo;
}

/**
 * Returns a data-URI-encoded rendering of an SVG
 * (since Qt fails to render inline SVGs at Retina resolution).
 */
QString VuoEditorUtilities::getHTMLForSVG(QString svgPath, int pointsWide, int pointsHigh)
{
	// QIcon::pixmap might not return the exact requested size
	// (e.g., if the icon's aspect ratio doesn't match the requested aspect ratio),
	// so create an exact pixmap and draw the icon on it.
	qreal dpr = QApplication::desktop()->devicePixelRatio();
	QPixmap pixmap(pointsWide * dpr, pointsHigh * dpr);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	QIcon(svgPath).paint(&painter, 0, 0, pointsWide * dpr, pointsHigh * dpr);

	QByteArray bytes;
	QBuffer buffer(&bytes);
	pixmap.save(&buffer, "PNG");
	return QString("<img src='data:image/png;base64,")
		+ bytes.toBase64()
		+ QString("' width=%1 height=%2 />")
			.arg(pointsWide)
			.arg(pointsHigh);
}

/**
 * Opens the user's Modules folder in Finder.
 */
void VuoEditorUtilities::openUserModulesFolder()
{
	QString path = QString::fromUtf8(VuoFileUtilities::getUserModulesPath().c_str());
	QDir dir(path);
	if (!dir.exists())
		QDir().mkpath(path);

	if (dir.exists())
		QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	else
		QMessageBox::information(NULL, QObject::tr("Vuo User Library folder"), QObject::tr("Please create this folder if you'd like to install nodes for this user account:\n\n%1").arg(path));
}

/**
 * Opens the system's Modules folder in Finder.
 */
void VuoEditorUtilities::openSystemModulesFolder()
{
	QString path = QString::fromUtf8(VuoFileUtilities::getSystemModulesPath().c_str());
	QDir dir(path);

	// Request authorization to create the folder.
	if (!dir.exists())
	{
		AuthorizationRef auth = NULL;
		if (AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &auth) == errAuthorizationSuccess)
		{
			AuthorizationItem right = {kAuthorizationRightExecute, 0, NULL, 0};
			AuthorizationRights rights = {1, &right};

			const char *promptString = QObject::tr("Vuo wants to create its System Library folder.").toUtf8().constData();
			AuthorizationItem prompt = {kAuthorizationEnvironmentPrompt, strlen(promptString), (void *)promptString, 0};
			AuthorizationEnvironment environment = {1, &prompt};

			if (AuthorizationCopyRights(auth, &rights, &environment,
										kAuthorizationFlagDefaults
										| kAuthorizationFlagInteractionAllowed
										| kAuthorizationFlagPreAuthorize
										| kAuthorizationFlagExtendRights,
										NULL) == errAuthorizationSuccess)
			{
				const char *args[] = { "-p", strdup(path.toUtf8().data()), NULL };
				FILE *pipe = NULL;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
				// See http://www.stevestreeting.com/2011/11/25/escalating-privileges-on-mac-os-x-securely-and-without-using-deprecated-methods/ for info on a non-deprecated path.
				if (AuthorizationExecuteWithPrivileges(auth, "/bin/mkdir", kAuthorizationFlagDefaults, (char * const *)args, &pipe) == errAuthorizationSuccess)
#pragma clang diagnostic pop
				{
					// Wait for `mkdir` to complete.
					int bytesRead = 0;
					do
					{
						char buffer[128];
						bytesRead = read(fileno(pipe), buffer, sizeof(buffer));
					} while (bytesRead);
					fclose(pipe);
				}
				free((char *)args[1]);
			}

			AuthorizationFree(auth, kAuthorizationFlagDefaults);
		}
	}

	if (dir.exists())
		QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	else
		QMessageBox::information(NULL, QObject::tr("Vuo System Library folder"), QObject::tr("Please create this folder if you'd like to install nodes for all users on this computer:\n\n%1").arg(path));
}

/**
 * Returns a boolean indicating whether the "Option" key was pressed in conjunction with
 * the provided @c event.
 *
 * This method works reliably even when the event originated from another application
 * while the Vuo Editor was inactive, e.g., when dragging and dropping files from the Finder.
 */
bool VuoEditorUtilities::optionKeyPressedForEvent(QEvent *event)
{
	// Technically we want to know whether the "Option" key was pressed at the time of the drop, not right at this moment,
	// but QGraphicsSceneDragDropEvent::modifiers() isn't providing that information reliably on OS X at Qt 5.3.
#ifdef __APPLE__
	bool optionKeyPressed = (CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState) & kCGEventFlagMaskAlternate);
	#else
	bool optionKeyPressed = (event->modifiers() & Qt::AltModifier);
	#endif

	return optionKeyPressed;
}

/**
 * Checks if the node class has source code that the user can edit, and if so, provides some information about it.
 *
 * @param nodeClass The node class to check.
 * @param[out] editLabel Translated text for a link or action to edit the source code, customized to the type of source code.
 * @param[out] sourcePath Path of the source code file.
 * @return True if there is a source code file to edit, false otherwise.
 */
bool VuoEditorUtilities::isNodeClassEditable(VuoNodeClass *nodeClass, QString &editLabel, QString &sourcePath)
{
	if (nodeClass->hasCompiler())
	{
		string expectedSourcePath = nodeClass->getCompiler()->getSourcePath();

		if (VuoFileUtilities::fileExists(expectedSourcePath))
		{
			string dir, file, ext;
			VuoFileUtilities::splitPath(expectedSourcePath, dir, file, ext);
			string sourceKind = (VuoFileUtilities::isCompositionExtension(ext) ?
									 "Composition" :
									 (VuoFileUtilities::isIsfSourceExtension(ext) ? "Shader" :
																					"Node"));

			editLabel = QObject::tr(("Edit " + sourceKind + "…").c_str());
			sourcePath = QString::fromStdString(expectedSourcePath);

			return true;
		}
	}

	return false;
}

/**
 * Checks whether a given node class is the recommended replacement for another given node class.
 *
 * @param oldNodeClass The node class to be replaced.
 * @param newNodeClass The potential replacement node class.
 * @return True if @c newNodeClass is the successor to @c oldNodeClass, false otherwise.
 */
bool VuoEditorUtilities::isNodeClassSuccessorTo(QString oldNodeClass, QString newNodeClass)
{
	map<QString, QString> successorForNodeClass;
	successorForNodeClass["vuo.mesh.make.lines"] = "vuo.scene.make.lines";
	successorForNodeClass["vuo.mesh.make.lineStrips"] = "vuo.scene.make.lineStrips";
	successorForNodeClass["vuo.mesh.make.parametric"] = "vuo.scene.make.parametric";
	successorForNodeClass["vuo.mesh.make.points"] = "vuo.scene.make.points";
	successorForNodeClass["vuo.mesh.make.sphere"] = "vuo.scene.make.sphere";
	successorForNodeClass["vuo.mesh.make.square"] = "vuo.scene.make.square";
	successorForNodeClass["vuo.mesh.make.triangle"] = "vuo.scene.make.triangle";

	// Case: newNodeClass is a designated successor to oldNodeClass
	if (successorForNodeClass[oldNodeClass] == newNodeClass)
		return true;

	// Case: oldNodeClass and newNodeClass have identical root names with increasing numerical suffixes, or
	// newNodeClass is identical to oldNodeClass plus a numerical suffix.
	QString root = "";
	QString oldSuffix = "";
	QString newSuffix = "";
	int minLength = qMin(oldNodeClass.length(), newNodeClass.length());
	for (int i = 0; (i < minLength); ++i)
	{
		if (oldNodeClass.at(i) == newNodeClass.at(i))
			root += oldNodeClass.at(i);
		else
			break;
	}

	oldSuffix = oldNodeClass.right(oldNodeClass.length()-root.length());
	newSuffix = newNodeClass.right(newNodeClass.length()-root.length());

	bool oldSuffixIsNumeric = false;
	int oldSuffixVal = oldSuffix.toInt(&oldSuffixIsNumeric);

	bool newSuffixIsNumeric = false;
	int newSuffixVal = newSuffix.toInt(&newSuffixIsNumeric);

	if (newSuffixIsNumeric && (oldSuffix.isEmpty() || (oldSuffixIsNumeric && (newSuffixVal > oldSuffixVal))))
	{
		// Exclude false positives where the numerical endings are meaningful parts of the root,
		// such as "vuo.osc.message.make.2" ->	"vuo.osc.message.make.3" and
		// "vuo.osc.message.get.1" -> "vuo.osc.message.get.11"
		bool rootEndsInNumberSegment = false;
		for (int i = root.length()-1; i >= 0; --i)
		{
			if (root.at(i).isDigit())
				continue;
			else
			{
				if (root.at(i) == '.')
					rootEndsInNumberSegment = true;
				break;
			}
		}

		return !rootEndsInNumberSegment;
	}

	return false;
}

/**
 * Returns a list of currently open composition- and code-editing windows, sorted case-insensitively by window title.
 */
QList<QMainWindow *> VuoEditorUtilities::getOpenEditingWindows()
{
	QList<QMainWindow *> openWindows;
	for (QWidget *openWidget : QApplication::topLevelWidgets())
		if ((dynamic_cast<VuoEditorWindow *>(openWidget) || dynamic_cast<VuoCodeWindow *>(openWidget)) && ! openWidget->isHidden())
			openWindows.append(static_cast<QMainWindow *>(openWidget));

	std::sort(openWindows.begin(), openWindows.end(), [](const QMainWindow *window1, const QMainWindow *window2) {
		return window1->windowTitle().remove("[*]").compare(window2->windowTitle().remove("[*]"), Qt::CaseInsensitive) < 0;
	});

	return openWindows;
}

/**
 * Returns a list of currently open composition-editing windows, sorted case-insensitively by window title.
 */
QList<VuoEditorWindow *> VuoEditorUtilities::getOpenCompositionEditingWindows()
{
	QList<VuoEditorWindow *> openCompositionWindows;
	for (QMainWindow *openWindow : getOpenEditingWindows())
		if (dynamic_cast<VuoEditorWindow *>(openWindow))
			openCompositionWindows.append(static_cast<VuoEditorWindow *>(openWindow));

	return openCompositionWindows;
}

/**
 * Returns a list of currently open composition- and code-editing windows, sorted by stacking order (frontmost first).
 */
QList<QMainWindow *> VuoEditorUtilities::getOpenEditingWindowsStacked()
{
	QList<QMainWindow *> openWindows = getOpenEditingWindows();

	std::sort(openWindows.begin(), openWindows.end(), [](const QMainWindow *window1, const QMainWindow *window2) {
		id nsView1 = (id)window1->winId();
		id nsWindow1 = objc_msgSend(nsView1, sel_getUid("window"));
		long orderedIndex1 = (long)objc_msgSend(nsWindow1, sel_getUid("orderedIndex"));

		id nsView2 = (id)window2->winId();
		id nsWindow2 = objc_msgSend(nsView2, sel_getUid("window"));
		long orderedIndex2 = (long)objc_msgSend(nsWindow2, sel_getUid("orderedIndex"));

		return orderedIndex1 < orderedIndex2;
	});

	return openWindows;
}

/**
 * Returns an open composition- or code-editing window editing @a filename, if one exists. Otherwise returns NULL.
 */
QMainWindow * VuoEditorUtilities::existingWindowWithFile(const QString &filename)
{
	QString canonicalFilePath = QFileInfo(filename).canonicalFilePath();

	for (QMainWindow *openWindow : getOpenEditingWindows())
		if (openWindow->windowFilePath() == canonicalFilePath)
			return openWindow;

	return NULL;
}

/**
 * Returns @ref VuoEditorWindow::getRaiseDocumentAction or @ref VuoCodeWindow::getRaiseDocumentAction.
 * A hack in place of proper polymorphism.
 */
QAction * VuoEditorUtilities::getRaiseDocumentActionForWindow(QMainWindow *window)
{
	VuoEditorWindow *editorWindow = dynamic_cast<VuoEditorWindow *>(window);
	if (editorWindow)
	{
		return editorWindow->getRaiseDocumentAction();
	}
	else
	{
		VuoCodeWindow *codeWindow = dynamic_cast<VuoCodeWindow *>(window);
		if (codeWindow)
			return codeWindow->getRaiseDocumentAction();
	}

	return nullptr;
}

/**
 * Hack in place of proper polymorphism.
 */
VuoRecentFileMenu * VuoEditorUtilities::getRecentFileMenuForWindow(QMainWindow *window)
{
	VuoEditorWindow *editorWindow = dynamic_cast<VuoEditorWindow *>(window);
	if (editorWindow)
	{
		return editorWindow->getRecentFileMenu();
	}
	else
	{
		VuoCodeWindow *codeWindow = dynamic_cast<VuoCodeWindow *>(window);
		if (codeWindow)
			return codeWindow->getRecentFileMenu();
	}

	return nullptr;
}

/**
 * Hack in place of proper polymorphism.
 */
QMenu * VuoEditorUtilities::getFileMenuForWindow(QMainWindow *window)
{
	VuoEditorWindow *editorWindow = dynamic_cast<VuoEditorWindow *>(window);
	if (editorWindow)
	{
		return editorWindow->getFileMenu();
	}
	else
	{
		VuoCodeWindow *codeWindow = dynamic_cast<VuoCodeWindow *>(window);
		if (codeWindow)
			return codeWindow->getFileMenu();
	}

	return nullptr;
}

/**
 * Hack in place of proper polymorphism.
 */
void VuoEditorUtilities::setWindowAsActiveWindow(QMainWindow *window)
{
	VuoEditorWindow *editorWindow = dynamic_cast<VuoEditorWindow *>(window);
	if (editorWindow)
	{
		return editorWindow->setAsActiveWindow();
	}
	else
	{
		VuoCodeWindow *codeWindow = dynamic_cast<VuoCodeWindow *>(window);
		if (codeWindow)
			return codeWindow->setAsActiveWindow();
	}
}

/**
  * Sets the canvas opacity level to the provided `opacity` value, where a
  * value of 0 means that the canvas should be fully transparent and
  * 255 means that it should be fully opaque.
  */
void VuoEditorUtilities::setWindowOpacity(QMainWindow *window, int opacity)
{
#ifdef __APPLE__
	id nsView = (id)window->winId();
	id nsWindow = objc_msgSend(nsView, sel_getUid("window"));
	objc_msgSend(nsWindow, sel_getUid("setAlphaValue:"), opacity/255.);
	#endif
}
