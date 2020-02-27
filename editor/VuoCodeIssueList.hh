/**
 * @file
 * VuoCodeIssueList interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCodeWindow;

/**
 * Renders a list of code issues.
 */
class VuoCodeIssueList : public QDockWidget
{
	Q_OBJECT

public:
	explicit VuoCodeIssueList(QWidget *parent);

	void updateIssues();
	void selectIssueForLine(int lineNumber);

private slots:
	void moveCursorToIssue(QListWidgetItem *item);

private:
	void updateColor(bool isDark);

	VuoCodeWindow *codeWindow;
	QListWidget *listWidget;

	friend VuoCodeWindow;
};
