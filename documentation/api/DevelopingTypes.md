@addtogroup DevelopingTypes

In a node, each data-and-event port has a @term{port type}, which is its type of data. Vuo comes with many built-in port types, such as @ref VuoInteger, @ref VuoReal, @ref VuoBoolean, and @ref VuoText. For a complete list of built-in port types, see @ref VuoTypes. You can also create your own custom port types. 

One reason you might want to create your own port type is to allow a node set to pass some structured data between nodes. For example, the @vuoNodeClass{vuo.image} node set uses a port type called @ref VuoImage to pass image data between nodes. You can connect a cable from a @ref VuoImage output port to a @ref VuoImage input port. 

Another reason you might want to create your own port type is to create an input port that provides a menu of options. For example, in the Vuo Editor, when you double-click on the constant flag for the @vuoNode{Make Wave} node's @vuoPort{wave} input port, it pops up a menu that lets you select the type of wave (sine, triangle, or sawtooth). The first step toward creating that menu was to define a port type called VuoWave and use it in the @vuoNode{Make Wave} node class, as described in this documentation. The other step was to create an input editor for the VuoWave type, as described in @ref DevelopingInputEditors. 



## Quick start

The easiest way to start developing a port type is with the example Qt project for a port type, which is provided with the Vuo SDK. 

   1. Install [Qt and Qt Creator](http://qt-project.org/downloads). 
   2. Make a copy of the example Qt project for a port type (`example/node/customType`). 
   3. Open the project in Qt Creator. 
   4. In `example.customType.pro`, change `ExampleLanguage.c` and `ExampleLanguage.h` to your port type file names. 
   5. In the example project folder, rename `ExampleLanguage.c` and `ExampleLanguage.h` to your port type file names. 
   6. In `example.customType.greet.c`, replace all occurrences of `ExampleLanguage` with your port type name. 
   7. Build your Qt project (Build > Build All). 
   8. Start the Vuo Editor (or restart it if it's already running). 

The next step is to modify the port type's source code so that it actually implements the data type you want. To learn how, read on. 


## Writing a port type

To implement a port type, you need to: 

   - Create the port type source files. 
   - Add <code>\#include "type.h"</code>. 
   - Call @ref VuoModuleMetadata to define the port type's metadata. 
   - Declare the port type's underlying data type. 
   - Declare and implement Vuo API functions to define the port type's behavior. 


### The file name

When you implement a port type, the first thing you have to decide is the type name. Every port type has two names: a type name (like @ref VuoInteger and a title (like "Integer"). The type name is how node classes refer to your port type. The title is what Vuo Editor users see when they hover over a port to see its information. When you create a port type, the file name is the type name. For example, the port type with name @ref VuoInteger is implemented in files called `VuoInteger.c` and `VuoInteger.h`. 


### The metadata

Here's an example of metadata for a port type: 

@code{.c}
VuoModuleMetadata({
					 "title" : "Integer",
					 "description" : "A signed 64-bit integer.",
					 "keywords" : [ "number", "signed" ],
					 "version" : "1.0.0"
				 });
@endcode

The @ref VuoModuleMetadata macro takes a [JSON-formatted](http://www.json.org/) argument. The argument includes the title for the port type, a version number to help users upgrade and manage their port types, a description, and an array of keywords. The description and keywords are not currently used. 

For more information, see the documentation for @ref VuoModuleMetadata. 


### The underlying data type

Your port type needs to be implemented with some C data type, such as a @c long, a @c char*, or a @c struct. Use a @c typedef to make your port type name an alias for this C data type. 

Here's one example: 

@code{.c}
typedef signed long VuoInteger;
@endcode

Here's another example: 

@code{.c}
typedef struct
{
	float x,y;
} VuoPoint2d;
@endcode

A port type can be composed of other port types. For example: 

@code{.c}
typedef struct
{
	VuoReal first,second;
} MyPairOfReals;
@endcode

If a port type refers to other port types, then you need to list those as dependencies in the @ref VuoModuleMetadata. For more information, see @ref ManagingDependencies.


### The functions

When you implement a port type, you have to implement certain functions to convert values of your port type to and from JSON, and to get a textual summary of a value. 

Unlike in a node class, in a port type each function name needs to be prefixed with the type name and an underscore. For example, the VuoInteger type has functions VuoInteger_makeFromJson(), VuoInteger_getJson(), and VuoInteger_getSummary(). In this documentation, we'll use "MyType" in place of the port type name. 

Every type needs to implement the MyType_makeFromJson() function and the MyType_getJson() function. These functions are responsible for converting a value of @c MyType to and from a [JSON-C](https://github.com/json-c/json-c) object. For each input port of type @c MyType in a composition, the MyType_makeFromJson() function is called when the composition starts in order to initialize the port's value. While the composition is running, MyType_makeFromJson() is also called each time the composition receives a message to change the input port's value — for example, when a Vuo Editor user edits an input port value. For each input or output port of type @c MyType, the MyType_getJson() function is called each time the composition receives a message to retrieve the port's value. 

Every type also needs to implement the MyType_getSummary() function, which returns a brief textual description of the port's value. This function is called each time the port's node receives an event. The summary appears in the Vuo Editor when the user hovers over a port to see its value while the composition is running. Be sure to keep the summary brief so that it can be efficiently sent many times per second to the Vuo Editor or another process. 

You can define additional functions for working with your port type. For example, many built-in port types define a @c MyType_make() function as a constructor that node classes and other port types can use to construct a value of @c MyType. Be sure to prefix your functions with the port type name to avoid naming conflicts with other port types. 

When you compile a port type, the Vuo compiler automatically adds some functions to it. These functions appear in the compiled port type file but not in the port type's source code. 

For more information about the port type functions, see the documentation for type.h. 

Also see the source code for Vuo's built-in port types, which can serve as examples to help you write your own port types. 



## Compiling a port type

Before you can install your port type, you need to use the Vuo Compiler to compile it to a `.bc` file. 

If you're using the example Qt project for creating a port type, then you can just build the Qt project (Build > Build All). 

Otherwise, you need to use the `vuo-compile` command-line tool that comes with the Vuo SDK. To learn how to use `vuo-compile`, see the [Vuo Manual](http://vuo.org/manual.pdf), run `vuo-compile --help`, or look at the `vuo-compile` command in the example Qt project. 


## Packaging a port type

The next step before installing your port type is to package it into a node set (@ref PackagingNodeSets). The node set should contain both the compiled (`.bc`) file and the header (`.h`) file for your port type. (Vuo needs the header file so it can specialize generic node classes like @vuoNode{Hold Value} and @vuoNode{Select Latest} with your port type.) 

If you're using the example Qt project for creating a port type, then when you build the project, the compiled port type is automatically packaged into a node set.

Otherwise, you need to zip the port type files into a node set, following the instructions in @ref PackagingNodeSets. 


## Installing a port type

The final step is to place your node set in the correct folder, so that it will be detected by the Vuo framework and the Vuo command-line tools. You can place it in either `~/Library/Application Support/Vuo/Modules/` or `/Library/Application Support/Vuo/Modules/`. For more information about these folders, see the [Vuo Manual](http://vuo.org/manual.pdf). 

If you're using the example Qt project for creating a port type, then when you build the project, the node set is automatically placed in `~/Library/Application Support/Vuo/Modules/`. 

Otherwise, you need to manually move the compiled port type (`.bc`) file to one of these folders. 

After that, you should be able to use your port type in a node class, just like the built-in port types. 



## Naming port types

**Please do not begin your port type's name with "Vuo".** This is reserved for port types distributed by Team Vuo / Kosada. Please use your own company or personal name for your port types so that Vuo users can appreciate your work (and not be confused). 

Built-in Vuo port types follow a set of naming conventions. If you develop port types to share with other Vuo developers, we encourage you to follow these conventions, too, to make your port types easier to use. 

   - A port type's name should:
      - Use upper camel case (`VuoFrameRequest`). 
      - Capitalize only the first letter of abbreviations (`VuoMidiNote`, not `VuoMIDINote`). 
   - A port type function's name should: 
      - Be prefixed with the port type name and an underscore (`VuoPoint2d_dotProduct()`). 
      - For the constructor function, be called "make" (`VuoPoint2d_make()`). 



## Using port types in interprocess communication

The JSON representation of a port value may either be used within a single process or be sent between separate processes. For example, the Vuo Editor sends and receives JSON representations of port values to and from a composition running in a separate process. Other applications that use Vuo's API may run a composition — and transmit JSON representations of port values — either to a separate process or within the current process (see @ref DevelopingApplications). For some port types, whether they're sent within-process or between-processes makes a difference in how they should be converted to and from JSON. 

For example, the VuoImage port type uses a shortcut to convert to and from JSON more efficiently within-process: its JSON format contains references to data stored in memory. However, this won't work when serializing a VuoImage in one process and unserializing it in another. The VuoImage port type needs another serialization method to fall back on when being sent between-processes. 

If you want your port type to handle JSON conversion differently depending on whether the JSON representation of the port value is being sent within-process or between-processes, you can implement the MyType_getJson() function to handle the within-process case and the MyType_getInterprocessJson() function to handle the between-processes case. Your implementation of the MyType_makeFromJson() function needs to handle both cases. 

For more information on using port types within-process and between-processes, see the documentation for MyType_getInterprocessJson(). See the VuoImage class for an example. 
