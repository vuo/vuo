/**
 * @file
 * VuoCodeEditor interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCodeEditor;

/**
 * Draws line numbers and error indicators in the left margin of a @ref VuoCodeEditor.
 */
class VuoCodeGutter : public QWidget
{
	Q_OBJECT

public:
	VuoCodeGutter(VuoCodeEditor *editor);

	void updateLineNumberFont();

private slots:
	void textChanged();

private:
	void paintEvent(QPaintEvent *event) override;
	void handleSliderMoved(int value);
	void resizeEvent(QResizeEvent *event) override;

	VuoCodeEditor *codeEditor;
	int scrollPosition;
	QFont lineNumberFont;
	static qreal leftMargin;
	static qreal rightMargin;
};
