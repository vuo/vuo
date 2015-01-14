@addtogroup DevelopingInputEditors

**Note:** Developers cannot create input editors yet.  We'll be finishing this up sometime during Vuo 0.5.x.

---

In the Vuo Editor, a user can edit the value of certain types of input ports by double-clicking on the port's constant flag, then using the widget that pops up. This widget is called an @term{input editor}. For example, the user can double-click on a @ref VuoText input port to pop up a text box, or a @ref VuoBoolean input port to pop up a menu, or a @ref VuoInteger input port to pop up a spin box or slider. If a port type doesn't already have a input editor, you can create one. 

An input editor is a [Qt plugin](http://qt-project.org/doc/qt-5.0/qtcore/plugins-howto.html#the-lower-level-api-extending-qt-applications) that implements the plugin interface defined by the VuoInputEditor class. 


## Quick start

@todo Explain how to use the example Qt project (https://b33p.net/kosada/node/5796)



## Writing an input editor

@todo (https://b33p.net/kosada/node/6229)

The Vuo SDK provides several derived classes of VuoInputEditor that you can derive from to create your own input editors. 

   - If your input editor is a menu, use VuoInputEditorWithMenu. 
   - If your input editor is a text field, use VuoInputEditorWithLineEdit. 
   - If your input editor is a dialog containing widgets, use VuoInputEditorWithDialog. 



## Building an input editor

@todo (https://b33p.net/kosada/node/6229)



## Installing an input editor

@todo (https://b33p.net/kosada/node/6229)



## Allowing a port to customize its input editor

Some input editors allow a port to specify how the input editor will be displayed. For example, the VuoInputEditorInteger input editor allows a @ref VuoInteger port to specify a minimum value, maximum value, and step size for its slider widget. If either the minimum or maximum is unspecified, the VuoInputEditorInteger instead displays a spin box widget. 

In the implementation of a node class, the VuoInputData macro's optional second argument is a [JSON-formatted](http://www.json.org/) set of details about the port. These details are passed to the VuoInputEditor::show function's @c details argument. Your input editor can use these details in any way you want to control how the input editor will be displayed. 

For example, VuoInputEditorInteger input editor uses JSON keys "suggestedMin", "suggestedMax", and "suggestedStep". A node class implementation can use those keys when defining a @ref VuoInteger port, for example: 
@code{.c}
void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) input
)
{
	...
}
@endcode
