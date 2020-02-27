/**
 * @file
 * VuoInputEditorText interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoTextEditor.hh"

/**
 * A VuoInputEditorText factory.
 */
class VuoInputEditorTextFactory : public VuoInputEditorFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorText.json")
	Q_INTERFACES(VuoInputEditorFactory)

public:
	virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays a text area for entering plain text.
 *
 * The text area has a fixed width but automatically adjusts its height (up to a point) to fit the text.
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "isCodeEditor" — If true, the input editor is adapted for editing source code (wider text area,
 *	   fixed-width font, Return and Tab keys type text instead of dismissing the input editor).
 *     Defaults to false.
 */
class VuoInputEditorText : public VuoTextEditor
{
	Q_OBJECT
};

