/**
 * @file
 * VuoCodeHighlighterGLSL interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCodeEditor;

/**
 * Applies GLSL 1.20 syntax highlighting.
 */
class VuoCodeHighlighterGLSL : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	VuoCodeHighlighterGLSL(QTextDocument *parent, VuoCodeEditor *codeEditor);

private:
	void highlightBlock(const QString &text);
	VuoCodeEditor *codeEditor;
};
