/**
 * @file
 * VuoCodeEditor interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCodeGutter;
class VuoCodeHighlighterGLSL;
class VuoCodeWindow;

/**
 * A widget for editing text code.
 */
class VuoCodeEditor : public QTextEdit
{
	Q_OBJECT

public:
	explicit VuoCodeEditor(QString initialSourceCode);

	void zoom11();
	void zoomIn();
	void zoomOut();
	bool isZoomedToActualSize();

	void selectLine(int lineNumber);

	void updateColor(bool isDark);

	QColor gutterColor;           ///< The gutter's background color.
	QColor gutterTextColor;       ///< The gutter's foreground color.
	QColor currentLineColor;      ///< The current line's background color.
	QColor operatorColor;         ///< The foreground color of GLSL operators.
	QColor commentColor;          ///< The foreground color of GLSL comments.
	QColor keywordColor;          ///< The foreground color of GLSL keywords.
	QColor builtinVariableColor;  ///< The foreground color of GLSL built-in variables.
	QColor builtinFunctionColor;  ///< The foreground color of GLSL built-in functions.
	QColor constantColor;         ///< The foreground color of GLSL constants.
	QColor preprocessorColor;     ///< The foreground color of GLSL preprocessor directives.
	QPixmap *errorIcon;           ///< The error indicator icon (red circle with exclamation).
	VuoCodeGutter *gutter;        ///< The gutter widget.

private:
	void setFontSize(int fontSize);
	void keyPressEvent(QKeyEvent *event);
	void handleTab(bool forward);
	void handleLinebreak(bool dontBreakAtCursor);
	void handleBlockEnd();
	void handleComment();
	int toggleLineComment(QTextCursor &cursor);
	void resizeEvent(QResizeEvent *event);
	void cursorPositionChanged();
	int getCurrentLineNumber();

	friend class VuoCodeGutter;
	friend class VuoCodeWindow;
	int currentFontSize;
	QList<int> fontSizes;
	VuoCodeHighlighterGLSL *highlighter;
	bool isDark;
};
