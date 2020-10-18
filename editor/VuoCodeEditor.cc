/**
 * @file
 * VuoCodeEditor implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeEditor.hh"

#include "VuoCodeGutter.hh"
#include "VuoCodeHighlighterGLSL.hh"
#include "VuoCodeIssueList.hh"
#include "VuoCodeWindow.hh"
#include "VuoEditor.hh"
#include "VuoInputEditorIcon.hh"
#include "VuoRendererColors.hh"

/**
 * Creates a new code editor widget.
 */
VuoCodeEditor::VuoCodeEditor(QString initialSourceCode):
	QTextEdit(),
	highlighter(nullptr)
{
	setAcceptRichText(false);

	setPlainText(initialSourceCode);

	VuoEditor *editor = static_cast<VuoEditor *>(qApp);
	isDark = editor->isInterfaceDark();
	updateColor(isDark);

	gutter = new VuoCodeGutter(this);

	// VuoCodeGutter doesn't yet support wrapping.
	setLineWrapMode(QTextEdit::NoWrap);

	fontSizes << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 16 << 20 << 28 << 36 << 48;
	zoom11();

	connect(this, &QTextEdit::cursorPositionChanged, this, &VuoCodeEditor::cursorPositionChanged);
	cursorPositionChanged();

	highlighter = new VuoCodeHighlighterGLSL(document(), this);
}

void VuoCodeEditor::setFontSize(int fontSize)
{
	currentFontSize = fontSize;

	// Menlo is included with Mac OS 10.7+, and includes italic and bold variants (unlike Monaco).
	QFont font("Menlo", fontSize);

	int spacesPerTab = 4;
	QFontMetricsF fontMetrics(font);
	qreal spaceWidth = fontMetrics.width(' ');
	qreal tabWidth = spacesPerTab * spaceWidth;
	setTabStopDistance(ceil(tabWidth));

	// Align characters to pixels.
	qreal pitch = (ceil(tabWidth) - tabWidth) / spacesPerTab;
	font.setLetterSpacing(QFont::AbsoluteSpacing, pitch);
	setFont(font);

	// A little thicker / more visible than the default 1px.
	setCursorWidth(fontSize/5.);

	gutter->updateLineNumberFont();
}

/**
 * Restores the default text size (100% aka 1:1).
 */
void VuoCodeEditor::zoom11()
{
	setFontSize(11);
}

/**
 * Makes the text larger.
 */
void VuoCodeEditor::zoomIn()
{
	foreach (int fontSize, fontSizes)
		if (fontSize > currentFontSize)
		{
			setFontSize(fontSize);
			break;
		}
}

/**
 * Makes the text smaller.
 */
void VuoCodeEditor::zoomOut()
{
	QList<int>::reverse_iterator i;
	for (i = fontSizes.rbegin(); i != fontSizes.rend(); ++i)
		if (*i < currentFontSize)
		{
			setFontSize(*i);
			break;
		}
}

/**
 * Returns true if the text is at its default size.
 */
bool VuoCodeEditor::isZoomedToActualSize()
{
	return currentFontSize == 11;
}

/**
 * Selects the text on the specified line.
 */
void VuoCodeEditor::selectLine(int lineNumber)
{
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNumber - 1);

	// Select the line, placing the cursor at the beginning of the line.
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
	cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);

	setTextCursor(cursor);
	setFocus();
}

void VuoCodeEditor::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();
	Qt::KeyboardModifiers modifiers = event->modifiers();

	if (key == Qt::Key_Tab)
		handleTab(true);
	else if (key == Qt::Key_BracketRight
		  && modifiers & Qt::ControlModifier
		  && !(modifiers & Qt::ShiftModifier))
		handleTab(true);
	else if (key == Qt::Key_BracketLeft
		  && modifiers & Qt::ControlModifier
		  && !(modifiers & Qt::ShiftModifier))
		handleTab(false);
	else if (key == Qt::Key_Backtab)
		handleTab(false);
	else if (key == Qt::Key_Return
		  && !((modifiers & Qt::AltModifier) || (modifiers & Qt::ControlModifier)))  // bubble up to VuoCodeWindow
		handleLinebreak(false);
	else if (key == Qt::Key_BraceRight)
		handleBlockEnd();
	else if (key == Qt::Key_Slash && modifiers & Qt::ControlModifier)
		handleComment();
	else
		QTextEdit::keyPressEvent(event);
}

/**
 * If text is selected, (un)indents the selection.
 * If no text is selected, just inserts a tab.
 */
void VuoCodeEditor::handleTab(bool forward)
{
	QTextCursor cursor = textCursor();

	if (!cursor.hasSelection())
	{
		if (forward)
			cursor.insertText("\t");
		return;
	}

	cursor.beginEditBlock();

	int selectionEnd = cursor.selectionEnd();
	cursor.setPosition(cursor.selectionStart());
	cursor.movePosition(QTextCursor::StartOfBlock);
	do {
		if (forward)
		{
			cursor.insertText("\t");
			++selectionEnd;
		}
		else
			if (toPlainText().at(cursor.position()) == '\t')
			{
				cursor.deleteChar();
				--selectionEnd;
				cursor.movePosition(QTextCursor::NextCharacter);
			}

		if (!cursor.movePosition(QTextCursor::NextBlock))
			break;
	} while (cursor.position() < selectionEnd);

	cursor.endEditBlock();
}

/**
 * Indents the new line to match the previous line,
 * taking into account opening curly braces.
 */
void VuoCodeEditor::handleLinebreak(bool dontBreakAtCursor)
{
	QTextCursor cursor = textCursor();
	cursor.beginEditBlock();

	if (dontBreakAtCursor)
		cursor.movePosition(QTextCursor::EndOfLine);
	cursor.insertText("\n");
	QTextCursor insertionCursor = cursor;

	// Get previous line's indent level.
	int indentLevel = 0;
	{
		cursor.movePosition(QTextCursor::PreviousBlock);
		while (toPlainText().at(cursor.position()) == '\t')
		{
			++indentLevel;
			if (!cursor.movePosition(QTextCursor::NextCharacter))
				break;
		}

		// Adjust indentation level if the line ends with a brace (excluding trailing whitespace).
		cursor.movePosition(QTextCursor::StartOfBlock);
		int startPos = cursor.position();
		cursor.movePosition(QTextCursor::EndOfBlock);
		while (toPlainText().at(cursor.position()).isSpace() && cursor.position() >= startPos)
			if (!cursor.movePosition(QTextCursor::PreviousCharacter))
				break;
		if (toPlainText().at(cursor.position()) == '{')
			++indentLevel;
	}

	for (int i = 0; i < indentLevel; ++i)
		insertionCursor.insertText("\t");

	cursor.endEditBlock();

	// Actually move the visible cursor.
	// If we do this before endEditBlock(), the viewport resets its scroll position.
	setTextCursor(insertionCursor);
}

/**
 * Reduces indent level if possible.
 */
void VuoCodeEditor::handleBlockEnd()
{
	QTextCursor cursor = textCursor();
	cursor.beginEditBlock();

	if (toPlainText().length()
		&& toPlainText().at(fmax(0, cursor.position() - 1)) == '\t')
		cursor.deletePreviousChar();

	cursor.insertText("}");

	cursor.endEditBlock();
}

/**
 * (Un)prefixes the current line with C-style comment brackets.
 * Returns the number of characters added or removed.
 */
int VuoCodeEditor::toggleLineComment(QTextCursor &cursor)
{
	cursor.movePosition(QTextCursor::StartOfBlock);

	while (cursor.position() < toPlainText().length()
		   && toPlainText().at(cursor.position()).isSpace())
		cursor.movePosition(QTextCursor::NextCharacter);

	if (toPlainText().mid(cursor.position(), 2) == "//")
	{
		cursor.deleteChar();
		cursor.deleteChar();
		return -2;
	}
	else
	{
		cursor.movePosition(QTextCursor::StartOfBlock);
		cursor.insertText("//");
		return 2;
	}
}

/**
 * Comments or uncomments text.
 */
void VuoCodeEditor::handleComment()
{
	QTextCursor cursor = textCursor();
	cursor.beginEditBlock();

	if (cursor.hasSelection())
	{
		int selectionEnd = cursor.selectionEnd();
		cursor.setPosition(cursor.selectionStart());
		do {
			int delta = toggleLineComment(cursor);
			selectionEnd += delta;
			if (!cursor.movePosition(QTextCursor::NextBlock))
				break;
		} while (cursor.position() < selectionEnd);
	}
	else
		toggleLineComment(cursor);

	cursor.endEditBlock();
}

void VuoCodeEditor::resizeEvent(QResizeEvent *event)
{
	QTextEdit::resizeEvent(event);
	gutter->resize(event->size());
}

int VuoCodeEditor::getCurrentLineNumber()
{
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::StartOfLine);

	int lines = 1;
	while (cursor.movePosition(QTextCursor::PreviousBlock))
		++lines;

	return lines;
}

void VuoCodeEditor::cursorPositionChanged()
{
	// Highlight the current line.
	QTextEdit::ExtraSelection selection;
	selection.format.setBackground(currentLineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = textCursor();
	selection.cursor.clearSelection();
	QList<QTextEdit::ExtraSelection> extraSelections;
	extraSelections.append(selection);
	setExtraSelections(extraSelections);


	if (parent())
	{
		VuoCodeWindow *codeWindow = static_cast<VuoCodeWindow *>(parent()->parent()->parent());
		if (codeWindow->issueList)
			codeWindow->issueList->selectIssueForLine(getCurrentLineNumber());
	}
}

/**
 * Applies dark mode rendering changes.
 */
void VuoCodeEditor::updateColor(bool isDark)
{
	VuoRendererColors c;
	c.setDark(isDark);

	QPalette p;

	QColor background = c.canvasFill();
	p.setColor(QPalette::Base, background);

	p.setColor(QPalette::Active,   QPalette::Highlight,       isDark ? "#12418c" : "#74acec");
	p.setColor(QPalette::Inactive, QPalette::Highlight,       isDark ? "#606060" : "#e0e0e0");
	p.setColor(QPalette::Active,   QPalette::HighlightedText, isDark ? "#c0c0c0" : "#404040");
	p.setColor(QPalette::Inactive, QPalette::HighlightedText, isDark ? "#c0c0c0" : "#404040");

	setPalette(p);

	currentLineColor = c.canvasFill().lighter(isDark ? 110 : 97);
	gutterColor      = c.canvasFill().lighter(isDark ? 150 : 90);
	gutterTextColor  = c.canvasFill().lighter(isDark ? 250 : 70);
	operatorColor    = isDark ? "#c0c0c0" : "#000000";
	commentColor     = isDark ? "#606060" : "#c0c0c0";

	{
		VuoRendererColors c(VuoNode::TintMagenta);
		c.setDark(isDark);
		keywordColor = c.nodeFrame();
	}

	{
		VuoRendererColors c(VuoNode::TintCyan);
		c.setDark(isDark);
		builtinVariableColor = c.nodeFrame();
	}

	{
		VuoRendererColors c(VuoNode::TintOrange);
		c.setDark(isDark);
		builtinFunctionColor = c.nodeFrame();
	}

	{
		VuoRendererColors c(VuoNode::TintBlue);
		c.setDark(isDark);
		constantColor = c.nodeFrame();
	}

	{
		VuoRendererColors c(VuoNode::TintYellow);
		c.setDark(isDark);
		preprocessorColor = c.nodeFrame();
	}

	errorIcon = VuoInputEditorIcon::renderErrorPixmap(isDark);

	setStyleSheet(VUO_QSTRINGIFY(
		QTextEdit {
			background: %1;
			color: %2;
		}
	).arg(background.name())
	 // Fade out normal text a little, so keywords and operators stand out more.
	 .arg(isDark ? "#a0a0a0" : "#606060"));

	if (highlighter && isDark != this->isDark)
	{
		this->isDark = isDark;

		highlighter->rehighlight();
		cursorPositionChanged();
	}
}
