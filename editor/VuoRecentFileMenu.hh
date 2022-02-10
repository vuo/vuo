/**
 * @file
 * VuoRecentFileMenu interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once


/**
 * Provides a "File > Open Recent" menu.
 */
class VuoRecentFileMenu : public QMenu
{
	Q_OBJECT

public:
	explicit VuoRecentFileMenu(QWidget *parent = 0);
	void addFile(const QString &filePath);
	QStringList getRecentFiles();
	void setRecentFiles(QStringList recentFiles);
	void pruneNonexistentFiles();
	void openMostRecentFile();

signals:
	void recentFileSelected(QString filePath); ///< Emitted when a file has been selected from the menu.
	void recentFileListCleared(); ///< Emitted when the user has cleared the list of recently opened files.

public slots:
	void clearRecentFileListActionTriggered();

private slots:
	void recentFileActionTriggered();

private:
	void updateRecentFileActions();
	static const int maxRecentFileCount;
	QStringList recentFiles;
};

