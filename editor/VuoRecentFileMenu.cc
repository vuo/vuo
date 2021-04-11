/**
 * @file
 * VuoRecentFileMenu implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRecentFileMenu.hh"
#include "VuoEditor.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"

const int VuoRecentFileMenu::maxRecentFileCount = 10;

/**
 * Creates an "Open Recent" menu.
 */
VuoRecentFileMenu::VuoRecentFileMenu(QWidget *parent) :
	QMenu(parent)
{
	setTitle(QApplication::translate("VuoEditorWindow", "Open Recent"));
	updateRecentFileActions();
}

/**
 * Adds file with path @c filePath to the list of recently opened files.
 */
void VuoRecentFileMenu::addFile(const QString &filePath)
{
	recentFiles.removeAll(filePath);
	recentFiles.prepend(filePath);

	while (recentFiles.size() > maxRecentFileCount)
		recentFiles.removeLast();

	updateRecentFileActions();
}

/**
 * Updates the internally stored list of recently opened files, removing
 * any files that no longer exist.
 */
void VuoRecentFileMenu::pruneNonexistentFiles()
{
	QStringList currentlyExistingRecentFiles = QStringList(recentFiles);
	bool foundMissingFile = false;

	// Filter out recent files that no longer exist.
	recentFiles.clear();
	foreach (QString recentFile, currentlyExistingRecentFiles)
	{
		QUrl url(recentFile);
		QString fileName;
		bool currentFileMissing = false;

		// Determine whether the current file in the list is an example composition.
		bool isExampleComposition = (url.isValid() && url.scheme() == VuoEditor::vuoExampleCompositionScheme);

		// If not, assume it is a regular composition.
		if (!isExampleComposition)
		{
			fileName = QFileInfo(recentFile).fileName();
			if (!VuoFileUtilities::fileExists(recentFile.toStdString()))
			{
				currentFileMissing = true;
				foundMissingFile = true;
			}
		}

		if (!currentFileMissing)
			recentFiles.append(recentFile);
	}

	if (foundMissingFile)
		updateRecentFileActions();
}

/**
 * Updates the "Open Recent" menu to reflect the internally stored list of
 * recently opened files.
 */
void VuoRecentFileMenu::updateRecentFileActions()
{
	clear();
	int currentNumRecentFiles = recentFiles.size();
	setEnabled(currentNumRecentFiles > 0);

	// List recently opened files.
	for (int i = 0; i < currentNumRecentFiles; ++i)
	{
		QAction *recentFileAction = new QAction(this);
		QString fileName, fileURL;

		QUrl url(recentFiles[i]);

		// Determine whether the current file in the list is an example composition.
		if (url.isValid() && url.scheme() == VuoEditor::vuoExampleCompositionScheme)
		{
			fileName = VuoStringUtilities::substrAfter(url.path().toUtf8().constData(), "/").c_str();
			fileURL = QFileInfo(recentFiles[i]).filePath();
		}

		// If not, assume it is a regular composition.
		else
		{
			fileName = QFileInfo(recentFiles[i]).fileName();
			fileURL = QString("file://").append(QFileInfo(recentFiles[i]).filePath());
		}

		recentFileAction->setText(fileName);
		recentFileAction->setData(fileURL);
		recentFileAction->setIcon(QIcon(":/Icons/vuo-composition.png"));
		if (i == 0)
			recentFileAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
		connect(recentFileAction, &QAction::triggered, this, &VuoRecentFileMenu::recentFileActionTriggered);
		addAction(recentFileAction);
	}

	// Add a "Clear Menu" option.
	if (currentNumRecentFiles > 0)
	{
		addSeparator();
		QAction *clearRecentFileListAction = new QAction(this);
		clearRecentFileListAction->setText(QApplication::translate("VuoEditorWindow", "Clear Menu"));
		connect(clearRecentFileListAction, &QAction::triggered, this, &VuoRecentFileMenu::clearRecentFileListActionTriggered);
		addAction(clearRecentFileListAction);
	}
}

/**
 * Emits a signal indicating that a file has been selected from the menu.
 */
void VuoRecentFileMenu::recentFileActionTriggered()
{
	QAction *sender = (QAction *)QObject::sender();
	emit recentFileSelected(sender->data().toString());
}

/**
 * Opens the most recently opened file.
 */
void VuoRecentFileMenu::openMostRecentFile()
{
	if (recentFiles.size())
	{
		QUrl url(recentFiles[0]);
		QString formattedURL;

		// Determine whether the current file in the list is an example composition.
		if (url.isValid() && url.scheme() == VuoEditor::vuoExampleCompositionScheme)
			formattedURL = QFileInfo(recentFiles[0]).filePath();

		// If not, assume it is a regular composition.
		else
			formattedURL = QString("file://").append(QFileInfo(recentFiles[0]).filePath());

		emit recentFileSelected(formattedURL);
	}
}

/**
 * Clears the list of recently opened files.
 */
void VuoRecentFileMenu::clearRecentFileListActionTriggered()
{
	recentFiles.clear();
	updateRecentFileActions();
	emit recentFileListCleared();
}

/**
 * Returns the list of recently opened files.
 */
QStringList VuoRecentFileMenu::getRecentFiles()
{
	return recentFiles;
}

/**
 * Sets the list of recently opened files.
 */
void VuoRecentFileMenu::setRecentFiles(QStringList recentFiles)
{
	this->recentFiles = recentFiles;
	updateRecentFileActions();
}
