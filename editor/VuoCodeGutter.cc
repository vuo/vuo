/**
 * @file
 * VuoCodeGutter implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeGutter.hh"

#include <sys/param.h>

#include "VuoCodeEditor.hh"
#include "VuoCodeEditorStages.hh"
#include "VuoCodeWindow.hh"
#include "VuoShaderIssues.hh"

qreal VuoCodeGutter::leftMargin = 5;
qreal VuoCodeGutter::rightMargin = 5;

/**
 * Creates a gutter.
 */
VuoCodeGutter::VuoCodeGutter(VuoCodeEditor *editor)
	: QWidget(editor)
{
	codeEditor = editor;
	scrollPosition = 0;

	lineNumberFont = QFont("Menlo");

	connect(codeEditor,                      &QTextEdit::textChanged,        this, &VuoCodeGutter::textChanged);
	connect(codeEditor->verticalScrollBar(), &QAbstractSlider::valueChanged, this, &VuoCodeGutter::handleSliderMoved);
}

/**
 * Updates the gutter's font size to match the main code editor.
 */
void VuoCodeGutter::updateLineNumberFont()
{
	lineNumberFont.setPointSize(codeEditor->font().pointSize() * .8);
	resizeEvent(NULL);
	update();
}

void VuoCodeGutter::paintEvent(QPaintEvent *event)
{
	QRect r = event->rect();

	QPainter painter(this);
	painter.fillRect(r, codeEditor->gutterColor);

	painter.setPen(codeEditor->gutterTextColor);
	painter.setFont(lineNumberFont);
	QFontMetrics lineNumberFontMetrics(lineNumberFont);
	int lineHeight = codeEditor->fontMetrics().height();
	int baselineAdjustment = codeEditor->fontMetrics().ascent() - lineNumberFontMetrics.ascent();
	int gutterWidth = width() - rightMargin;

	// QFontMetrics.lineHeight is in pixels, but QTextEdit doesn't quantize lines to pixels,
	// so we can't just draw at multiples of lineHeight.
	// Instead, get the actual position of each line from the QTextLayout.
	QTextCursor cursor = codeEditor->textCursor();
	cursor.movePosition(QTextCursor::Start);
	QTextBlock block = cursor.block();
	int viewportHeight = codeEditor->viewport()->geometry().height();
	int lineNumber = 0;

	VuoCodeWindow *codeWindow = static_cast<VuoCodeWindow *>(window());
	if (codeWindow->issues)
	{
		vector<VuoShaderIssues::Issue> issues = codeWindow->issues->issuesForStage(codeWindow->stages->currentStage());
		// The issue list is assumed to be sorted by line number.
		// Therefore we only need to advance through it once, instead of once for every line number.
		vector<VuoShaderIssues::Issue>::iterator issueIterator = issues.begin();

		int iconSize = codeEditor->errorIcon->height() / window()->devicePixelRatio();

		do
		{
			++lineNumber;
			qreal y = block.layout()->position().y() - scrollPosition;
			if (y < -lineHeight)
				continue;
			if (y > viewportHeight)
				break;

			painter.drawText(QRectF(0, y + baselineAdjustment + 1, gutterWidth, lineHeight),
							 Qt::AlignRight, QString::number(lineNumber));

			bool haveIssue = false;
			while (issueIterator != issues.end())
			{
				if (issueIterator->lineNumber == lineNumber)
				{
					haveIssue = true;
					break;
				}
				if (issueIterator->lineNumber > lineNumber)
					break;
				++issueIterator;
			}
			if (haveIssue)
				painter.drawPixmap(leftMargin, y + (lineHeight - iconSize) / 2, iconSize, iconSize, *codeEditor->errorIcon);
		} while ((block = block.next()).isValid());
	}
}

void VuoCodeGutter::handleSliderMoved(int value)
{
	scrollPosition = value;
	update();
}

void VuoCodeGutter::textChanged()
{
	resizeEvent(NULL);
	update();
}

void VuoCodeGutter::resizeEvent(QResizeEvent *event)
{
	int lineNumberDigits = MAX(2, log10(codeEditor->document()->lineCount()) + 1);
	QFontMetricsF lineNumberFontMetrics(lineNumberFont);
	int iconSize = codeEditor->errorIcon->height() / window()->devicePixelRatio();
	int width = leftMargin + iconSize + leftMargin + lineNumberFontMetrics.horizontalAdvance('0') * lineNumberDigits + rightMargin;

	if (width == size().width())
		return;

	QRect c = codeEditor->rect();
	setGeometry(QRect(c.left(), c.top(), width, c.height()));

	codeEditor->setViewportMargins(width, 0, 0, 0);

	if (codeEditor->parent())
	{
		VuoCodeEditorStages *stages = static_cast<VuoCodeEditorStages *>(codeEditor->parent()->parent());
		stages->updatePosition();
	}
}
