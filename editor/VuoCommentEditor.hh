/**
 * @file
 * VuoCommentEditor interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoTextEditor.hh"

/**
 * An input editor that displays a text area for entering comment text.
 *
 * The text area has a fixed width but automatically adjusts its height (up to a point) to fit the text.
 */
class VuoCommentEditor : public VuoTextEditor
{
	Q_OBJECT

public:
	VuoCommentEditor(void);
	using VuoInputEditorWithDialog::show;
	json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details);
	bool supportsTabbingBetweenPorts(void);
	void setWidth(int width);
	void setHeight(int height);

private slots:
	void resizeToFitText(void);

private:
	static const int commentMargin;

	int width;
	int height;

	bool getCodeEditor(json_object *details);
};
