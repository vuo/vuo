/**
 * @file
 * VuoNodeLibraryTextFilter interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A widget for filtering the nodes displayed in the Node Library.
 */
class VuoNodeLibraryTextFilter : public QLineEdit
{
	Q_OBJECT
public:
	explicit VuoNodeLibraryTextFilter(QWidget *parent = 0);
	~VuoNodeLibraryTextFilter();

signals:
	void nodeLibraryTextFilterFocusLost(); ///< Emitted when the text filter loses focus.
	void nodeLibraryReceivedPasteCommand(); ///< Emitted when the text filter receives the keyboard shortcut for the 'Paste' command.

public slots:

protected:
	void resizeEvent(QResizeEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void focusOutEvent(QFocusEvent *event);

private slots:
	void updateUI();
	void updateColor(bool isDark);

private:
	QToolButton *searchButton;
	QToolButton *clearButton;
	QIcon searchIcon;
	QIcon clearIcon;
	QIcon clearIconPressed;
	bool inGraphiteDisplayMode();

};

