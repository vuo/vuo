/**
 * @file
 * VuoCodeIssueList implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeIssueList.hh"

#include "VuoCodeEditor.hh"
#include "VuoCodeEditorStages.hh"
#include "VuoCodeWindow.hh"
#include "VuoEditor.hh"
#include "VuoShaderIssues.hh"

/**
 * Creates an issue list widget.
 */
VuoCodeIssueList::VuoCodeIssueList(QWidget *parent)
	: QDockWidget(parent)
{
	codeWindow = static_cast<VuoCodeWindow *>(parent);

	setFeatures(QDockWidget::NoDockWidgetFeatures);
	setFixedHeight(0);

	// Hide the titlebar.
	setTitleBarWidget(new QWidget);

	listWidget = new QListWidget(this);
	setWidget(listWidget);

	listWidget->setIconSize(QSize(12,12));

#ifdef __APPLE__
	// Disable standard OS X focus 'glow'
	listWidget->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

	connect(listWidget, &QListWidget::currentItemChanged, this, &VuoCodeIssueList::moveCursorToIssue);

	VuoEditor *editor = static_cast<VuoEditor *>(qApp);
	connect(editor, &VuoEditor::darkInterfaceToggled, this, &VuoCodeIssueList::updateColor);
	updateColor(editor->isInterfaceDark());
}

/**
 * Updates the widget contents to match the editor's issue list.
 */
void VuoCodeIssueList::updateIssues()
{
	QIcon errorIcon = *codeWindow->stages->someEditor()->errorIcon;

	listWidget->clear();
	for (vector<VuoShaderIssues::Issue>::iterator it = codeWindow->issues->issues().begin(); it != codeWindow->issues->issues().end(); ++it)
	{
		QListWidgetItem *item = new QListWidgetItem(errorIcon, QString::fromStdString(it->message), codeWindow->issueList->listWidget);
		item->setData(Qt::UserRole,   it->stage);
		item->setData(Qt::UserRole+1, it->lineNumber);
	}

	setFixedHeight(codeWindow->issues->issues().empty() ? 0 : fmin(listWidget->sizeHintForRow(0) * listWidget->count() + 2, 200));
}

void VuoCodeIssueList::moveCursorToIssue(QListWidgetItem *item)
{
	if (!item)
		return;

	VuoShaderFile::Stage stage = static_cast<VuoShaderFile::Stage>(item->data(Qt::UserRole).toInt());
	codeWindow->stages->switchToStage(stage);

	int lineNumber = item->data(Qt::UserRole+1).toInt();
	if (!VuoShaderIssues::isUserEnteredLine(lineNumber))
		return;

	VuoCodeEditor *codeEditor = static_cast<VuoCodeEditor *>(codeWindow->stages->currentWidget());
	codeEditor->selectLine(lineNumber);
}

/**
 * Highlights the issue corresponding to the given line number (if any).
 */
void VuoCodeIssueList::selectIssueForLine(int lineNumber)
{
	VuoShaderFile::Stage stage = codeWindow->stages->currentStage();

	bool matched = false;
	int itemCount = listWidget->count();
	for (int i = 0; i < itemCount; ++i)
	{
		VuoShaderFile::Stage itemStage = static_cast<VuoShaderFile::Stage>(listWidget->item(i)->data(Qt::UserRole).toInt());
		int itemLineNumber = listWidget->item(i)->data(Qt::UserRole+1).toInt();
		if (itemStage == stage
		 && itemLineNumber == lineNumber)
		{
			disconnect(listWidget, &QListWidget::currentItemChanged, this, &VuoCodeIssueList::moveCursorToIssue);
			listWidget->setCurrentRow(i);
			connect(listWidget, &QListWidget::currentItemChanged, this, &VuoCodeIssueList::moveCursorToIssue);

			matched = true;
			break;
		}
	}

	if (!matched)
		listWidget->setCurrentItem(NULL);
}

void VuoCodeIssueList::updateColor(bool isDark)
{
	QPalette p;

	VuoCodeEditor *e = codeWindow->stages->someEditor();

	QColor background = e->gutterColor.lighter(isDark ? 50 : 150);
	p.setColor(QPalette::All,      QPalette::Base,            background);

	// Fade out normal text a little, so keywords and operators stand out more.
	p.setColor(QPalette::All,      QPalette::Text,            isDark ? "#a0a0a0" : "#606060");

	p.setColor(QPalette::Active,   QPalette::Highlight,       isDark ? "#12418c" : "#74acec");
	p.setColor(QPalette::Inactive, QPalette::Highlight,       isDark ? "#606060" : "#e0e0e0");
	p.setColor(QPalette::Active,   QPalette::HighlightedText, isDark ? "#c0c0c0" : "#404040");
	p.setColor(QPalette::Inactive, QPalette::HighlightedText, isDark ? "#c0c0c0" : "#404040");

	listWidget->setPalette(p);

	setStyleSheet(VUO_QSTRINGIFY(
		QListWidget {
			font-size: 12pt;
			border: none;
			border-top: 3px solid %1;
			background: %1;
		}
	).arg(background.name()));
}
