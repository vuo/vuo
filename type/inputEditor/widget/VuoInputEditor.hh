/**
 * @file
 * VuoInputEditor interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITOR_HH
#define VUOINPUTEDITOR_HH

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

class VuoInputEditor;

/**
 * Plugin interface for input editors. To create a custom input editor for a port type, create a
 * <a href="http://qt-project.org/doc/qt-5.0/qtcore/plugins-howto.html#the-lower-level-api-extending-qt-applications">Qt plugin</a>
 * that implements a derived class of VuoInputEditorFactory and a derived class of VuoInputEditor.
 *
 * @see DevelopingInputEditors
 */
class VuoInputEditorFactory : public QObject
{
public:
	/**
	 * Returns a new instance of a derived class of VuoInputEditor.
	 */
	virtual VuoInputEditor * newInputEditor(void) = 0;
};

/**
 * Declares VuoInputEditorFactory to be an interface for plugins.
 */
Q_DECLARE_INTERFACE(VuoInputEditorFactory, "org.vuo.inputEditorFactory/1.0");

/**
 * This class implements the input editor widget displayed by the Vuo Editor when editing an input port value of a certain type.
 *
 * @see DevelopingInputEditors
 */
class VuoInputEditor : public QObject
{
	Q_OBJECT

public:
	/**
	 * Displays the input editor and waits until the user has finished or canceled editing.
	 *
	 * @param portLeftCenter The left center point (in global coordinates) of the port whose value is to be edited.
	 *		Used to position the input editor.
	 * @param originalValue The value to display initially in the input editor.
	 * @param details Additional details about the port (e.g., suggested minimum and maximum values).
	 *		A port in a node class implementation can optionally provide these details.
	 *		To assist node class developers, the input editor's class description should document
	 *		the JSON keys recognized by the input editor.
	 * @param portNamesAndValues *Not yet implemented.* The name and value of each input port that is on the same node
	 *      as the port being edited by this input editor (including that port) and that has a constant value
	 *      (no incoming data cable). Together with @a details, this information can be used to limit the input editor's
	 *      range based on other input ports' values.
	 *
	 * @see VuoTypes for information about types and their serialization to JSON.
	 */
	virtual json_object * show(QPoint portLeftCenter, json_object *originalValue, json_object *details, map<QString, json_object *> portNamesAndValues) = 0;

	/**
	 * Returns true if this input editor should be part of the Vuo Editor's tab order when using
	 * Tab / Shift-Tab to navigate between input ports.
	 *
	 * If true, this input editor should emit tabbedPastLastWidget() and tabbedBackwardPastFirstWidget()
	 * signals when appropriate. If false, those signals are ignored.
	 *
	 * Unless overridden, this function always returns false.
	 */
	virtual bool supportsTabbingBetweenPorts(void);

	static QFont getDefaultFont(void);
	static QString getDefaultFontCss(void);


signals:
	/**
	 * An input editor can emit this signal to inform the Vuo Editor that the value has been edited.
	 *
	 * If the Vuo Editor receives this signal while a composition is running, it updates the running
	 * composition immediately to use the updated value (while the input editor is still showing).
	 *
	 * @param newValue The updated value.
	 */
	void valueChanged(json_object *newValue);

	/**
	 * If supportsTabbingBetweenPorts() returns true, an input editor should emit this signal when
	 * the Tab key is pressed while the last widget in the input editor's tab order has focus.
	 * This tells the Vuo Editor to navigate to the next port in the Vuo Editor's tab order.
	 */
	void tabbedPastLastWidget();

	/**
	 * If supportsTabbingBetweenPorts() returns true, an input editor should emit this signal when
	 * the Shift-Tab key combination is pressed while the first widget in the input editor's tab order has focus.
	 * This tells the Vuo Editor to navigate to the previous port in the Vuo Editor's tab order.
	 */
	void tabbedBackwardPastFirstWidget();
};

#endif // VUOINPUTEDITOR_HH
