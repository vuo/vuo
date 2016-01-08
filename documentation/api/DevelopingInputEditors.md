@addtogroup DevelopingInputEditors

In the Vuo Editor, a user can edit the value of certain types of input ports by double-clicking on the port's constant flag, then using the widget that pops up. This widget is called an @term{input editor}. For example, the user can double-click on a @ref VuoText input port to pop up a text box, or a @ref VuoBoolean input port to pop up a menu, or a @ref VuoInteger input port to pop up a spin box or slider. If a port type doesn't already have an input editor, you can create one. 


## Quick start

The easiest way to start developing an input editor is with the example Qt project provided with the Vuo SDK. This example project includes a port type, an input editor for the port type, and a node class that demonstrates the input editor. 

First, be able to build the example project as-is: 

   1. Install [Qt and Qt Creator](http://qt-project.org/downloads). 
   2. Make a copy of the example Qt project for an input editor (`example/node/customType`). 
   3. Open the project in Qt Creator. 
   4. Set `VUO_FRAMEWORK_PATH` and `QT_ROOT` to the correct paths for your system. 
   5. Build your Qt project (Build > Build All). 

The next time you start Vuo Editor, the @vuoNodeClass{example.customType.greet} node class should appear in the Node Library, and the node's @vuoPort{language} input port should have a menu input editor that lets you select a language. 

To create your own input editor based on the example project:

   1. Implement your node classes and port types, placing them in the project folder (see @ref DevelopingNodeClasses and @ref DevelopingTypes). 
   2. Remove `example.customType.greet.c`, `ExampleLanguage.h`, and `ExampleLanguage.c`. 
   3. In all project file names and contents, replace `ExampleLanguage` with the name of your port type (e.g. `MyType`). 
   4. In `MyTypeInputEditor.cc`, change the implementation of @c MyTypeInputEditor::setUpMenuTree() to use your port type. 


## Writing an input editor

An input editor is a [Qt plugin](http://qt-project.org/doc/qt-5.0/qtcore/plugins-howto.html#the-lower-level-api-extending-qt-applications) that implements the plugin interface defined by the @ref VuoInputEditorFactory class. To do this, you need to implement a derived class of VuoInputEditorFactory and a derived class of VuoInputEditor. 

You can either derive from the @ref VuoInputEditor class directly, or you can derive one of the classes provided in the Vuo SDK (`framework/resources/inputEditorWidgets`) for your convenience: 

   - For a menu, use @ref VuoInputEditorWithMenu. 
   - For a text field, use @ref VuoInputEditorWithLineEdit. 
   - For a dialog containing widgets, use @ref VuoInputEditorWithDialog. 

In addition to the derived classes, the input editor needs a JSON-formatted metadata file containing the name of the port type that this input editor can edit. For an example of the correct format, see `ExampleLanguageInputEditor.json` in the example Qt project. 


## Building an input editor

Since the Vuo Editor loads its input editor plugins at runtime, you need to build your input editor to a dynamic library. 

Typically, you'll want to do this by building the input editor with a Qt project that uses `CONFIG += plugin` in its `.pro` file. For an example, see the example Qt project (`example/node/customType`). 


## Installing an input editor

Since the Vuo Editor looks for input editor plugins in `~/Library/Application Support/Vuo/Modules/` and `/Library/Application Support/Vuo/Modules/`, you need to place your built input editor in one of these folders. For more information about these folders, see the [Vuo Manual](http://vuo.org/manual.pdf). 

If you're using the example Qt project, then when you build the project, the compiled port type is automatically placed in `~/Library/Application Support/Vuo/Modules/`. 

Otherwise, you need to manually move the built input editor (`.dylib`) file to one of these folders. 

After that, the next time you start the Vuo Editor, your input editor should pop up when you double-click on any input port of that port type. 



## Allowing a port to customize its input editor

Some input editors allow a port to specify how the input editor will be displayed. For example, the VuoInputEditorInteger input editor allows a @ref VuoInteger port to specify a minimum value, maximum value, and step size for its slider widget. If either the minimum or maximum is unspecified, the VuoInputEditorInteger instead displays a spin box widget. 

In the implementation of a node class, the VuoInputData macro's optional second argument is a [JSON-formatted](http://www.json.org/) set of details about the port. These details are passed to the VuoInputEditor::show function's @c details argument. Your input editor can use these details in any way you want to control how the input editor will be displayed. 

For example, the VuoInputEditorReal input editor uses JSON keys "suggestedMin", "suggestedMax", and "suggestedStep". A node class implementation can optionally use those keys when defining a @ref VuoReal port, for example: 
@code{.c}
void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":1.0}) input
)
{
	...
}
@endcode
