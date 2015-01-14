@addtogroup DevelopingNodeClasses

A node class is like a function or method. It takes some input, processes it, and produces some output. 

When you write a node class using Vuo's API, the node class consists of: 

   - A macro call that defines metadata about the node class, such as its title and description. 
   - One or more function definitions, which define how the node class processes its inputs to produce its outputs. 

Vuo currently provides an API for the C programming language. (Node classes can also reference code in C++, Objective-C, and any other language that exports C symbols; see @ref DevelopingLibraryModules and @ref ManagingDependencies.) Support for other programming languages is planned. 



## Quick start

The easiest way to start developing a node class is with one of the example Qt projects for a node class, which are provided with the Vuo SDK. 

   1. Install [Qt and Qt Creator](http://qt-project.org/downloads). 
   2. Make a copy of the example Qt project for a stateless node class (`example/node/stateless`). 
   3. Open the project in Qt Creator. 
   4. In `stateless.pro`, change `test.vuoize.c` to your node class file name. 
   5. In the example project folder, rename `test.vuoize.c` to your node class file name. 
   6. In `test.vuoize.c`, in the @ref VuoModuleMetadata call, change "Vuoize Text" to the title that you want to appear on the node. 
   7. Build your Qt project (Build > Build All). 
   8. Start the Vuo Editor (or restart it if it's already running). 

Your node class should now be listed in your Node Library. You can now add it to a composition and see it perform the task defined 
in its nodeEvent() function. 

The next step is to modify the nodeEvent() function so that your node actually does something interesting. To learn how, read on. 



## Writing a node class

To implement a node class, you need to: 

   - Create the node class source file. 
   - Add <code>\#include "node.h"</code>. 
   - Call @ref VuoModuleMetadata to define the node class's metadata. 
   - Implement Vuo API functions to define the node class's behavior. 


### The file name

When you implement a node class, the first thing you have to decide is the class name. Every node class has two names: a class name (like @vuoNodeClass{vuo.math.add.integer}) and a title (like @vuoNode{Add}). In the Vuo Editor, you can see the difference between titles and class names by switching between the "Titles" view and the "Class names" view of the Node Library. After a node has been added to a composition, its title can be changed, but its class name always stays the same. When you create a node class, the file name is the node class name. For example, the node class with name @vuoNodeClass{vuo.math.add.integer} is implemented in a file called `vuo.math.add.integer.c`. 


### The metadata

Here's an example of metadata for a node class: 

@code{.c}
VuoModuleMetadata({
					 "title" : "Add",
					 "description" : "Adds the terms and outputs their sum.",
					 "keywords" : [ "sum", "+" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });
@endcode

The @ref VuoModuleMetadata macro takes a [JSON-formatted](http://www.json.org/) argument. The argument includes the default title for nodes of this node class, a description to help users understand how to use the node class, an array of keywords used when searching the Node Library, and a version number to help users upgrade and manage their node classes. 

Node classes are not the only files in Vuo that can define module metadata. You'll see this in @ref DevelopingTypes and @ref DevelopingLibraryModules. The title, description, keywords, and version can be defined in other types of files as well. 

But some module metadata are specific to node classes. The module metadata above include whether the node class is an interface, which affects how the node is rendered in a composition. 

When deciding which keywords to put in the module metadata, be aware that some are added automatically, so you don't have to add them. Each word in the node's title and class name is automatically used in searches of the Node Library. If your node class has trigger ports, then the keywords "events", "trigger", and "fire" are automatically added. 

For more information, see the documentation for @ref VuoModuleMetadata. 


### The functions

When you implement a node class, you have to implement certain functions that define what happens when a node receives an event. You implement different functions depending on whether the node class is stateless or stateful. 

A stateless node class (like @vuoNode{Add}) is simpler. It just has input ports and output ports; given the same input, a stateless node always produces the same output. A stateful node class (like @vuoNode{Count}) additionally keeps track of its state. It can remember its progress, and it can fire events when something happens in the background. 


#### A stateless node class

For a stateless node class, you just need to implement one function: @ref nodeEvent. The @ref nodeEvent function is called each time a node of your node class receives an event through any input port. (If the same event is received through multiple input ports, the @ref nodeEvent function is  called just one time.) Its arguments represent input and output ports of the node. 

As a simple example, here's the @ref nodeEvent function for the @vuoNodeClass{vuo.math.subtract.integer} node class: 

@code{.cc}
void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) a,
		VuoInputData(VuoInteger, {"default":0}) b,
		VuoOutputData(VuoInteger) difference
)
{
	*difference = a - b;
}
@endcode

The above @ref nodeEvent function has a @ref VuoInputData parameter for each input port and a @ref VuoOutputData parameter for each output port. (The refresh port and done port are added to each node automatically.) Both @ref VuoInputData and @ref VuoOutputData define data-and-event ports. The parameter name (@vuoPort{a}, @vuoPort{b}, or @vuoPort{difference}) is the port name that appears on the node. 

The first argument of @ref VuoInputData or @ref VuoOutputData is the port type. Vuo comes with many built-in port types (see @ref VuoTypes), and you can also define your own (see @ref DevelopingTypes). 

The second argument of @ref VuoInputData defines details about the input port in JSON format. In this case, it specifies the default value for the input port when a node is first added to a composition. 

The body of @ref nodeEvent defines what happens when a node receives an event. In this case, it subtracts the two input port values and sets the output port value. Notice that @c difference is a *pointer* to the @ref VuoOutputData type (@ref VuoInteger). 


#### A stateful node class

For a stateful node class, there are several functions to implement. 

Instead of @ref nodeEvent, you need to implement an equivalent function called @ref nodeInstanceEvent. Like @ref nodeEvent, the @ref nodeInstanceEvent function is called whenever the node receives an event. 

You also need to implement the @ref nodeInstanceInit function and the @ref nodeInstanceFini function to set up and clean up the node's state. The @ref nodeInstanceInit function is called when the composition starts or when the node is added to a running composition. The @ref nodeInstanceFini function is called when the composition stops or when the node is removed from a running composition. 

If your node class has trigger ports, you may also need to implement the @ref nodeInstanceTriggerStart function, the @ref nodeInstanceTriggerUpdate function, and the @ref nodeInstanceTriggerStop function. 

As an example, let's look at a simplified version of the @vuoNodeClass{vuo.math.count.real} node class. Its only input port is @vuoPort{increment}. Here are the functions for that node class: 

@code{.c}
VuoReal * nodeInstanceInit()
{
	VuoReal *countState = (VuoReal *) malloc(sizeof(VuoReal));
	VuoRegister(countState, free);
	*countState = 0;
	return countState;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoReal *) countState,
		VuoInputData(VuoReal, {"default":1}) increment,
		VuoInputEvent(VuoPortEventBlocking_None,increment) incrementEvent,
		VuoOutputData(VuoReal) count
)
{
	if (incrementEvent)
		**countState += increment;
	*count = **countState;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoReal *) countState
)
{
}
@endcode

The state of the node is called @term{instance data}. It's created by @ref nodeInstanceInit and passed, via @ref VuoInstanceData parameters, to @ref nodeInstanceEvent and @ref nodeInstanceFini. Notice that the return type of @ref nodeInstanceInit and the type passed to each @ref VuoInstanceData call must match. Currently, it needs to be a pointer type; support for non-pointer types is planned. Unlike the type passed to @ref VuoInputData or @ref VuoOutputData, the type passed to @ref VuoInstanceData doesn't have to be a port type. For example, it can be a struct type defined in your node class. The name of the @ref VuoInstanceData parameter (@c countState) isn't important; you can choose any name. Notice that @c countState is a *pointer* to the @ref VuoInstanceData type (@c VuoReal*). 

The call to @ref VuoRegister in @ref nodeInstanceInit is necessary to make sure that the memory allocated for @c countState is freed at the right time. For more information, see @ref ManagingMemory. 

Besides the instance data, another difference between this example and the stateless example above is the @ref VuoInputEvent parameter of @ref nodeInstanceEvent. The @ref VuoInputEvent macro can represent either an event-only port or the event part of a data-and-event port. In this case, it's the latter. We can see this because the second argument of @ref VuoInputEvent is the name of the corresponding @ref VuoInputData parameter (and the port name that appears on the node). The first argument of @ref VuoInputEvent says whether the port has an event wall, an event door, or (as in this case) neither. The value of @c incrementEvent is true if the node has just received an event through its @vuoPort{increment} port. 

The body of @ref nodeInstanceEvent defines what happens when the node receives an event. In this case, it increments the count if the event came in through the @vuoPort{increment} port, then outputs the count. 


#### @anchor DevelopingNodeClassesImmutable Nodes should treat port data as immutable

The Vuo Compiler assumes that data passed between nodes is immutable.

If a node function receives a heap type (pointer) as input, it _should not_ modify that heap data.  For example, this is incorrect:
@code{.c}
void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoOutputData(VuoText) textWithFirstLetterCapitalized
)
{
	// Don't do this! 
	text[0] = toupper(text[0]);
	*textWithFirstLetterCapitalized = text;
}
@endcode

Instead, node functions should make a copy of the heap data, then modify that copy.  For example:
@code{.c}
void nodeEvent
(
		VuoInputData(VuoText) text,
		VuoOutputData(VuoText) textWithFirstLetterCapitalized
)
{
	VuoText t = VuoText_make(text);  // makes a copy of text
	t[0] = toupper(t[0]);
	*textWithFirstLetterCapitalized = t;
}
@endcode

Also, if a stateful node function sends heap data via an output port, it _should not_ keep a reference to that heap data and modify it after the node function returns.

See @ref ManagingMemory for more information.


#### Additional functions and variables

In addition to the Vuo API functions, your node class can define additional functions. Be sure to add a unique prefix to these function names to avoid naming conflicts with other node classes. Although the Vuo compiler renames the @ref nodeEvent function and other API functions to avoid naming conflicts, it doesn't rename your custom functions. 

If you want to share functions between multiple node classes, or use C++ or Objective-C functions in a node class, see @ref DevelopingLibraryModules. 

A node class *should not* define any global variables. 


### Advanced topics

For more information about the parameters that can be passed to the @ref nodeEvent, @ref nodeInstanceEvent, and other node class functions — including how to create a trigger port — see @ref VuoNodeParameters. 

For more information about the node class functions, see @ref VuoNodeMethodsStateless and @ref VuoNodeMethodsStateful. 

Also see the source code for Vuo's built-in node classes, which can serve as examples to help you write your own node classes. 



## Compiling a node class

Before you can install your node class, you need to use the Vuo Compiler to compile it to a `.vuonode` file. 

If you're using the example Qt project for creating a node class, then you can just build the Qt project (Build > Build All). 

Otherwise, you need to use the `vuo-compile` command-line tool that comes with the Vuo SDK. To learn how to use `vuo-compile`, see the [Vuo Manual](http://vuo.org/manual.pdf), run `vuo-compile --help`, or look at the `vuo-compile` command in the example Qt project. 



## Installing a node class

The final step is to place your compiled node class in the correct folder, so that it will show up in the Vuo Editor's Node Library and be detected by the Vuo framework and the Vuo command-line tools. You can place it in either `~/Library/Application Support/Vuo/Modules/` or `/Library/Application Support/Vuo/Modules/`. For more information about these folders, see the [Vuo Manual](http://vuo.org/manual.pdf). 

If you're using the example Qt project for creating a node class, then when you build the project, the compiled node class is automatically placed  in `~/Library/Application Support/Vuo/Modules/`. 

Otherwise, you need to manually move the compiled node class (`.vuonode`) file to one of these folders. 

After that, the next time you start the Vuo Editor, the node class should show up in the Vuo Editor's Node Library. You can also see a list of all installed node classes by running `vuo-compile --list-node-classes`. 



## Troubleshooting

If you're having trouble compiling or installing a node class, try running `vuo-compile --verbose`. This lists the paths that Vuo is using to compile a node class and find installed node classes. 

If your node class is not showing up in the Vuo Editor's Node Library: 

   - Try restarting the Vuo Editor.  
   - Try changing the Node Library to "Class names" view and searching for your node class file name. 
   - Make sure the compiled node class (`.vuonode`) file is in `~/Library/Application Support/Vuo/Modules/` or `/Library/Application Support/Vuo/Modules/`. 
   - Make sure there are no warnings when you compile the node class. 
   - Make sure the node class has @ref VuoModuleMetadata. 


## Naming node classes and ports

**Please do not begin your node class's name with "vuo".** This is reserved for node classes distributed by Team Vuo / Kosada. Please use your own company or personal name for your node classes so that Vuo users can appreciate your work (and not be confused). 

Built-in Vuo nodes follow a set of naming conventions. If you develop node classes to share with other Vuo users, we encourage you to follow these conventions, too, to make your node classes easier to use. 

   - A node class's title should: 
      - Consist of a verb phrase (@vuoNode{Subtract}, not @vuoNode{Difference}). 
      - Use [title case (Chicago Manual of Style)](http://blog.winepresspublishing.com/2012/06/grammar-tip-how-capitalize-titles/) (@vuoNode{Count within Range}). 
   - A node class's class name should: 
      - Consist of several parts separated by dots: 
         - The first part is the creator ("vuo" in @vuoNodeClass{vuo.math.wave}, "mycompany" in @vuoNodeClass{mycompany.image.frobnicate}). 
         - If the node class belongs to a set of related node classes, the next part is the node set name ("math" in @vuoNodeClass{vuo.math.wave}). 
         - The next part is the action performed by the node class, usually similar to the title ("wave" in @vuoNodeClass{vuo.math.wave}).
         - If there are details about the action performed, such as the data type acted on, these come last ("integer" in @vuoNodeClass{vuo.math.add.integer}, "cube" in @vuoNodeClass{vuo.scene.make.cube}). 
         - In summmary, the format is: @vuoNodeClass{[creator].[optional node set].[action].[optional details]}. 
      - Use lower camel case (@vuoNodeClass{vuo.math.isLessThan.integer}). 
   - A port's name should:
      - Consist of full words or phrases, not abbreviations (@vuoPort{increment}, not @vuoPort{incr}). 
      - If the port is a trigger port, consist of a past-tense verb phrase (@vuoPort{started}, @vuoPort{receivedNote}, @vuoPort{requestedFrame}). 
      - If the port is an input port that causes something special to happen when it receives an event, consist of a present-tense verb phrase (@vuoPort{sendNote}, @vuoPort{increment}). 
      - Use lower camel case (@vuoPort{wrapMode}). 
      - Be descriptive — especially if the node may be used as a type converter, which hides the node's title (@vuoPort{roundedInteger}, not @vuoPort{integer}). 

In addition to these general rules, there are some special kinds of node classes that all have similar names: 

   - A "Make" node puts together pieces (the inputs) to create a structured data value (the output). 
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].make.[optional details]} (example: @vuoNodeClass{vuo.color.make.rgb}). 
      - Its title should have the form @vuoNodeClass{Make [optional details] [thing made]} (example: @vuoNodeClass{Make RGB Color}). 
   - A "Get" node decomposes a structured data value (the input) into pieces (the outputs). 
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].get.[optional details]} (example: @vuoNodeClass{vuo.color.get.rgb}). 
      - Its title should have the form @vuoNodeClass{Get [optional details] [thing unmade] Values} (example: @vuoNodeClass{Get RGB Color Values}). 


## Type-converter nodes

In the Vuo Editor, when the user attempts to connect a cable between ports of different types (for example, @ref VuoInteger to @ref VuoReal), the connection is bridged by a type-converter node (for example, @vuoNode{Convert Integer to Real}) if one is available. This type-converter node is rendered in a special collapsed form and is attached to the input port to which it's connected. Currently, only certain built-in nodes may be used as type converters. Support is planned to allow any node with a single data-and-event input port and a single data-and-event output port of a different type to be used as a type converter. The Vuo Renderer decides when a node should be rendered as a collapsed, attached type converter, based on its connections to other nodes. 
