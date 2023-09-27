@addtogroup DevelopingNodeClasses

Vuo comes with many built-in node classes, listed in the Node Library. If you want to create your own node class, there are several ways to do it:

   - Create a [subcomposition](https://doc.vuo.org/@vuoVersion/manual/using-subcompositions-inside-of-other-compositions.xhtml).
   - Write an [ISF graphics shader](https://doc.vuo.org/@vuoVersion/manual/turning-graphics-shaders-into-nodes.xhtml).
   - Write a node class in C/C++/Objective-C — the subject of this documentation.

## Getting started

### Option 1: Source file in Modules folder

The quickest way to get started is to grab the source code file from one of the example projects provided in the Vuo SDK:

   1. [Install the Vuo SDK](https://doc.vuo.org/@vuoVersion/manual/the-command-line-tools.xhtml#installing-the-vuo-sdk).
   2. In Vuo, go to Tools > Open User Library in Finder.
   3. Copy `/Library/Developer/Vuo/example/node/stateless/example.stateless.vuoize.c` to the Modules folder you just opened.
   4. Rename the copied file to `me.test.first.c`.
   5. Open the file in a text editor or IDE.
   6. In the @ref VuoModuleMetadata call, change "Vuoize Text" to "My Node Class" and save the file.

Your node class should now be listed in Vuo's Node Library, and you should be able to use it in compositions. (If not, check Tools > Show Console for errors.)

### Option 2: Qt Creator project

If you chose Option 1, you might be wondering what the rest of the files in the example project are for. To use them, you have to do a little more setup initially, but it pays off as you develop the node class because you get better IDE support and error reporting.

   1. [Install the Vuo SDK](https://doc.vuo.org/@vuoVersion/manual/the-command-line-tools.xhtml#installing-the-vuo-sdk).
   2. Install [Qt and Qt Creator](https://www.qt.io/offline-installers).
   3. Copy `/Library/Developer/Vuo/example/node/stateless` to your Desktop.
   4. Open the copied project in Qt Creator.
   5. Rename `example.stateless.vuoize.c` to `me.test.first.c`.
   6. In `CMakeLists.txt`, change `example.stateless.vuoize.c` to `me.test.first.c`.
   7. In `me.test.first.c`, in the @ref VuoModuleMetadata call, change "Vuoize Text" to "My Node Class".
   8. Build your project (Build > Build All).

### Option 3: CMake project

Or if you like the benefits of Option 2 but you'd prefer to use the command line and your own IDE or text editor instead of Qt Creator:

   1. [Install the Vuo SDK](https://doc.vuo.org/@vuoVersion/manual/the-command-line-tools.xhtml#installing-the-vuo-sdk).
   2. Copy `/Library/Developer/Vuo/example/node/stateless` to your Desktop.
   3. Rename `example.stateless.vuoize.c` to `me.test.first.c`.
   4. In `CMakeLists.txt`, change `example.stateless.vuoize.c` to `me.test.first.c`.
   5. In `me.test.first.c`, in the @ref VuoModuleMetadata call, change "Vuoize Text" to "My Node Class".
   6. In Terminal:
      - `cd ~/Desktop/stateless`
      - `mkdir build`
      - `cd build`
      - `cmake ..`
      - `make`

## Writing a node class

To implement a node class, you need to: 

   - Create the node class source file. 
   - Call @ref VuoModuleMetadata to define the node class's metadata.
   - Implement Vuo API functions to define the node class's behavior.

### The file name

Every node class has two names: [the title and the node class name](https://doc.vuo.org/@vuoVersion/manual/nodes-are-your-building-blocks.xhtml). For example, the node with title @vuoNode{Add} has class name @vuoNodeClass{vuo.math.add}. When you create a node class, the file name is the node class name plus a file extension. For example, @vuoNodeClass{vuo.math.add} node class is implemented in a file called `vuo.math.add.c`.

You should choose a file extension corresponding to the programming language in which the node class is implemented — `.c` for C, `.cc` for C++, `.m` for Objective-C, `.mm` for Objective-C++.

### The metadata

Here's an example of metadata for a node class: 

@code{.c}
VuoModuleMetadata({
					 "title" : "Add",
					 "description" : "Adds the terms and outputs their sum.",
					 "keywords" : [ "sum", "+" ],
					 "version" : "1.0.0"
				 });
@endcode

The @ref VuoModuleMetadata macro takes a [JSON-formatted](https://www.json.org/) argument. The argument includes the default title for nodes of this node class, a description to help users understand how to use the node class, an array of keywords used when searching the Node Library, and a version number to help users upgrade and manage their node classes.

Node classes are not the only files in Vuo that can define module metadata. You'll see this in @ref DevelopingTypes and @ref DevelopingLibraryModules. The title, description, keywords, and version can be defined in other types of files as well. 

For more information, see the documentation for @ref VuoModuleMetadata. 

#### Keywords

When deciding which keywords to put in the module metadata, be aware that some are added automatically, so you don't have to add them:

   - Each word in the node's title and class name is automatically used in searches of the Node Library.
   - If your node class has trigger ports, then the keywords "events", "trigger", and "fire" are automatically added.
   - If your node title begins with "Receive", the keywords "i/o", "interface", "input", and "provider" are automatically added.
   - If your node title begins with "Send", the keywords "i/o", "interface", "output", and "consumer" are automatically added.

Multi-word phrases are permitted as keywords, but for purposes of Node Library searches will be treated as if each word were a distinct keyword.

### The functions

When you implement a node class, you have to implement certain functions that define what happens when a node receives an event. You implement different functions depending on whether the node class is stateless or stateful. 

A stateless node class (like @vuoNode{Add}) is simpler. It just has input ports and output ports; given the same input, a stateless node always produces the same output. A stateful node class (like @vuoNode{Count}) additionally keeps track of its state. It can remember its progress, and it can fire events when something happens in the background. 

If using C++, you'll need to enclose the @ref VuoModuleMetadata call and Vuo API functions in `extern "C" { … }` so that the Vuo compiler can recognize them.

#### A stateless node class

For a stateless node class, you just need to implement one function: @ref nodeEvent. The @ref nodeEvent function is called each time a node of your node class receives an event through any input port. (If the same event is received through multiple input ports, the @ref nodeEvent function is  called just one time.) Its arguments represent input and output ports of the node. 

As a simple example, here's the @ref nodeEvent function for the @vuoNodeClass{vuo.logic.negate} node class: 

@code{.cc}
void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":false}) value,
		VuoOutputData(VuoBoolean) notValue
)
{
	*notValue = !value;
}
@endcode

The above @ref nodeEvent function has a @ref VuoInputData parameter for each input port and a @ref VuoOutputData parameter for each output port. Both @ref VuoInputData and @ref VuoOutputData define data-and-event ports. The parameter name (@vuoPort{value} or @vuoPort{notValue}) is turned into the port name that appears on the node ("Value" or "Not Value").

The first argument of @ref VuoInputData or @ref VuoOutputData is the data type. Vuo comes with many built-in types (see @ref VuoTypes), and you can also define your own (see @ref DevelopingTypes).

The second argument of @ref VuoInputData defines details about the input port in JSON format. In this case, it specifies the default value for the input port when a node is first added to a composition. 

The body of @ref nodeEvent defines what happens when a node receives an event. In this case, it negates the input port value and sets the output port value. Notice that @c notValue is a *pointer* to the @ref VuoOutputData type (@ref VuoBoolean). 

#### A stateful node class

For a stateful node class, there are two required functions to implement, and a few optional functions.

Instead of @ref nodeEvent, you need to implement an equivalent function called @ref nodeInstanceEvent. Like @ref nodeEvent, the @ref nodeInstanceEvent function is called whenever the node receives an event. 

The difference between a stateless and a stateful node class is that the stateful node class stores its state as @term{instance data}. You need to implement the @ref nodeInstanceInit function to set up the instance data. The @ref nodeInstanceInit function is called when the composition starts or when the node is added to a running composition.

If you need to tear down the instance data when the node class is finished using it, you should implement the @ref nodeInstanceFini function. (Vuo takes care of some tear-down automatically, as described later in this section.) The @ref nodeInstanceFini function is called when the composition stops or when the node is removed from a running composition.

If your node class has trigger ports, you may also need to implement the @ref nodeInstanceTriggerStart function, the @ref nodeInstanceTriggerUpdate function, and the @ref nodeInstanceTriggerStop function. 

As an example, let's look at a simplified version of the @vuoNodeClass{vuo.math.count} node class. Its input ports are @vuoPort{increment} and @vuoPort{decrement}. Its instance data stores the current count. Here are the functions for that node class:

@code{.c}
VuoInteger nodeInstanceInit
(
		VuoInputData(VuoInteger) setCount
)
{
	return setCount;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoInteger) countState,
		VuoInputData(VuoInteger, {"default":1}) increment,
		VuoInputEvent({"data":"increment"}) incrementEvent,
		VuoInputData(VuoInteger, {"default"1}) decrement,
		VuoInputEvent({"data":"decrement"}) decrementEvent,
		VuoOutputData(VuoInteger) count
)
{
	if (incrementEvent)
		*countState += increment;
	if (decrementEvent)
		*countState -= decrement;
	*count = *countState;
}
@endcode

The @ref nodeInstanceInit function sets up the instance data. For this example, the only setup required is to initialize the instance data to the current value of the @vuoPort{setCount} port. The @ref nodeInstanceInit function returns the initial value of the instance data, which Vuo stores and later passes to other functions of the node class. If there are multiple @vuoNode{Count} nodes in a composition, Vuo stores a separate instance data for each one.

The first time an event hits the node, the @ref nodeInstanceEvent function is called, and the instance data that was created by @ref nodeInstanceInit is passed in through the `countState` argument. As you may have guessed, this means that the return type of @ref nodeInstanceInit must match the data type in the @ref VuoInstanceData macro. (Here that data type happens to be a Vuo type, @ref VuoInteger, but other C data types are also allowed.) The name of the parameter (`countState`) isn't important; you can choose any name you like.

If the event has hit the @vuoPort{increment} or @vuoPort{decrement} input port, the code in the @ref nodeInstanceEvent function modifies both the @vuoPort{count} output port's value and the instance data's value. As mentioned in the previous section, the @ref VuoOutputData parameter `count` is actually a *pointer* to a @ref VuoInteger. Similarly, the @ref VuoInstanceData parameter `countState` is a *pointer* to a @ref VuoInteger.

The next time an event hits the node, the @ref nodeInstanceEvent function is called again, and the instance data that was modified on the previous call to @ref nodeInstanceEvent is passed back in through the `countState` argument.

This node class doesn't have a @ref nodeInstanceFini function because there's no need for it. The instance data allocated by @ref nodeInstanceInit is a simple data type that Vuo knows how to deallocate, and does so automatically. If the instance data were more complex, then a @ref nodeInstanceFini function might be needed to deallocate it; see @ref ManagingMemory. If the node class opened a file or device handle, started a timer, etc., then @ref nodeInstanceFini would need to close, stop, or otherwise clean up the resources used.

The simplified @vuoNode{Count} example introduces one other new element, which can appear in either stateless or stateful nodes: the @ref VuoInputEvent macro. A function parameter with @ref VuoInputEvent can represent either an event-only port or the event part of a data-and-event port. In this case, it's the latter. We can see this because of the "data" key in the JSON-formatted argument of @ref VuoInputEvent. The value for that key is the name of the corresponding @ref VuoInputData parameter for a data-and-event port. The `incrementEvent` and `decrementEvent` arguments are booleans that are true if the node has just received an event through that input port, and false otherwise.

#### Nodes should treat port data as immutable {#DevelopingNodeClassesImmutable}

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

In addition to the Vuo API functions, your node class can define additional functions. Be sure to make these functions `static` to avoid naming conflicts with other node classes. Although the Vuo compiler renames the @ref nodeEvent function and other API functions to avoid naming conflicts, it doesn't rename your custom functions.

If you want to share functions between multiple node classes, or use C++ or Objective-C functions in a node class, see @ref DevelopingLibraryModules. 

A node class *should not* define any global variables. 

#### Generic data types

If a node class can work with multiple different types of data, then rather than implementing a version of the node class for each data type, you can use @term{generic types} as placeholders for the actual data types. When a node with generic types is added to a composition, each of its generic types has the potential to be specialized (replaced) with an actual data type. Some examples of node classes that use generic types are @vuoNode{Hold Value}, @vuoNode{Select Latest}, and @vuoNode{Add}. 

To use a generic type in a node class, use @c VuoGenericType1 in place of the data type. For example, you can define a generic input port as `VuoInputData(VuoGenericType1) value`. You can define a list-of-generics input port as `VuoInputData(VuoList_VuoGenericType1) list`. You can append a value of generic type to a list by calling `VuoListAppend_VuoGenericType1(list, value)`. 

To use more than one generic type in a node class, you can use @c VuoGenericType2, @c VuoGenericType3, and so on. 

To specify the default value for a generic input port, you can use the "defaults" key (as opposed to the the singular "default" key for non-generic input ports) in the JSON-formatted port details. For example, a port that can be specialized to either a @ref VuoReal defaulting to 1 or a @ref VuoPoint2d defaulting to (1,1) would be declared with this parameter: `VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1, "VuoPoint2d":{"x":1,"y":1}}}) portName`.

For a node class to use generic types, it must be part of a node set. The source code for the node class and its included header files must be packaged into the node set. See @ref PackagingNodeSets. 

For more information about using generic types in a node class — including how to restrict the types that a generic type can be specialized with — see @ref VuoModuleMetadata.

### Advanced topics

For more information about the parameters that can be passed to the @ref nodeEvent, @ref nodeInstanceEvent, and other node class functions — including how to create a trigger port — see @ref VuoNodeParameters. 

For more information about the node class functions, see @ref VuoNodeMethodsStateless and @ref VuoNodeMethodsStateful. 

Also see the source code for Vuo's built-in node classes, which can serve as examples to help you write your own node classes. 

## Compiling and installing a node class

The quickest way to compile and install a node class is to place its source file in one of Vuo's [Modules folders](https://doc.vuo.org/@vuoVersion/manual/installing-a-node.xhtml). Vuo automatically compiles the source file and adds the node class to the Node Library. Any errors or warnings are reported in Vuo's console.

You may prefer to compile the node class as a separate step, so that you can easily spot errors or warnings in your IDE or Terminal. You can start with one of the example projects for node classes provided in the Vuo SDK; see the Getting Started section above. When you build any of those projects, it will use the `vuo-compile` command-line tool to compile the node class, then copy the resulting `.vuonode` file to the User Modules folder. (Run `vuo-compile --help` to see a list of options.)

## Troubleshooting

If you're having trouble compiling or installing a node class, try running `vuo-compile` with the `--verbose` flag. This lists the paths that Vuo is using to compile a node class and find installed node classes.

If your node class is not showing up in the Vuo editor's Node Library:

   - Try restarting the Vuo editor.
   - Try changing the Node Library to the "Display by class" view and searching for your node class name.
   - Make sure the node class is installed in a [Modules folder](https://doc.vuo.org/@vuoVersion/manual/installing-a-node.xhtml) where Vuo can find it. If it's in a composition-local Modules folder, try moving it to System or User Modules.
   - Make sure there are no errors or warnings when you compile the node class.
   - Make sure the node class has VuoModuleMetadata.

## Naming node classes and ports

**Please do not begin your node class's name with "vuo".** This is reserved for node classes distributed by Team Vuo / Kosada. Please use your own company or personal name for your node classes so that Vuo users can appreciate your work (and not be confused). 

Built-in Vuo nodes follow a set of naming conventions. If you develop node classes to share with other Vuo users, we encourage you to follow these conventions, too, to make your node classes easier to use. 

   - A node class's title should: 
      - Consist of a verb phrase (@vuoNode{Subtract}, not @vuoNode{Difference}). 
      - Use [title case (Chicago Manual of Style)](https://web.archive.org/web/20130311235940/http://blog.winepresspublishing.com/2012/06/grammar-tip-how-capitalize-titles/) (@vuoNode{Count within Range}).
   - A node class's class name should: 
      - Consist of several parts separated by dots: 
         - The first part is the creator ("vuo" in @vuoNodeClass{vuo.math.count}, "mycompany" in @vuoNodeClass{mycompany.image.frobnicate}). 
         - If the node class belongs to a set of related node classes, the next part is the node set name ("math" in @vuoNodeClass{vuo.math.count}). 
         - The next part describes the action performed by the node class, usually using one or more words from the title ("count" in @vuoNodeClass{vuo.math.count} / @vuoNode{Count}, "send" in @vuoNodeClass{vuo.audio.send} / @vuoNode{Send Live Audio}, "volume" in @vuoNodeClass{vuo.audio.volume} / @vuoNode{Adjust Volume}). 
         - If the node class is one of several that perform variations on the same action, the last part distinguishes this node class ("rgb" in @vuoNodeClass{vuo.color.make.rgb}, "duration" in @vuoNodeClass{vuo.motion.smooth.duration}). 
         - In summmary, the format is: @vuoNodeClass{[creator].[optional node set].[action].[optional details]}. 
      - Use lower camel case (@vuoNodeClass{vuo.math.isLessThan}). 
   - A port's name should:
      - Use lower camel case (@vuoPort{wrapMode}). Vuo automatically turns camel-case names into capitalized port names with spaces. 
      - Consist of full words or phrases, not abbreviations (@vuoPort{increment}, not @vuoPort{incr}). 
      - If the port is a trigger port, consist of a past-tense verb phrase (@vuoPort{started}, @vuoPort{receivedNote}, @vuoPort{requestedFrame}). 
      - If the port has a port action, consist of a present-tense verb phrase (@vuoPort{sendNote}, @vuoPort{increment}). 
      - Be descriptive — especially if the node may be used as a type converter, which hides the node's title (@vuoPort{roundedInteger}, not @vuoPort{integer}). 

In addition to these general rules, there are some special kinds of node classes that all have similar names: 

   - A "Receive" node inputs data from a device or data source into the composition.
      - If it's the only "Receive" node in its node set, its node class name should have the form @vuoNodeClass{[creator].[node set].receive} (example: @vuoNodeClass{vuo.audio.receive}). 
      - Its title should begin with "Receive" (example: @vuoNodeClass{Receive Live Audio}). 
   - A "Send" node outputs data from the composition to a device, file, or network.
      - If it's the only "Send" node in its node set, its node class name should have the form @vuoNodeClass{[creator].[node set].send} (example: @vuoNodeClass{vuo.audio.send}). 
      - Its title should begin with "Send" (example: @vuoNodeClass{Send Live Audio}). 
   - A "Specify" node describes a device so that a "Receive" or "Send" node can select a device matching that description.
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].specify.[optional details]} (example: @vuoNodeClass{vuo.serial.specify.name}, @vuoNodeClass{vuo.audio.specify.input.model}). (Node classes created before we established this convention use the term "make" instead of "specify", but "specify" is now preferred.)
      - Its title should have the form @vuoNodeClass{Specify [thing made] [optional details]} (example: @vuoNodeClass{Specify Serial Device by Name}, @vuoNodeClass{Specify Audio Input by Model}).
   - A "Make" node puts together pieces (the inputs) to create a structured data value (the output). 
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].make.[optional details]} (example: @vuoNodeClass{vuo.color.make.rgb}). 
      - Its title should have the form @vuoNodeClass{Make [optional details] [thing made]} (example: @vuoNodeClass{Make RGB Color}). 
   - A "Get" node decomposes a structured data value (the input) into pieces (the outputs). 
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].get.[optional details]} (example: @vuoNodeClass{vuo.color.get.rgb}). 
      - Its title should have the form @vuoNodeClass{Get [optional details] [thing unmade] Values} (example: @vuoNodeClass{Get RGB Color Values}). 
   - A "Filter" node blocks events to its (non-list) first input port based on filter parameters specified by other ports.
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].filter.[optional thing filtered].[parameter]} (example: @vuoNodeClass{vuo.osc.filter.address}). 
      - Its title should have the form @vuoNodeClass{Filter [optional thing filtered] by [parameter]} (example: @vuoNodeClass{Filter by Address}). 
   - A "Find" node reduces its first input port (a list) by removing some of its items based on parameters specified by other ports.
      - Its node class name should have the form @vuoNodeClass{[creator].[node set].find.[optional thing found].[parameter]} (example: @vuoNodeClass{vuo.leap.find.hand.confidence}, @vuoNodeClass{vuo.syphon.find.server.app}). 
      - Its title should have the form @vuoNodeClass{Find [thing found] by [parameter]} (example: @vuoNodeClass{Find Hands by Confidence}, @vuoNodeClass{Find Servers by App}). 

## Type-converter nodes

In the Vuo editor, when the user attempts to connect a cable between ports of different types (for example, @ref VuoInteger to @ref VuoReal), the connection is bridged by a type-converter node (for example, @vuoNode{Convert Integer to Real}) if one is available. This type-converter node is rendered in a special collapsed form and is attached to the input port to which it's connected. Currently, only certain built-in nodes may be used as type converters. Support is planned to allow any node with a single data-and-event input port and a single data-and-event output port of a different type to be used as a type converter. The Vuo Renderer decides when a node should be rendered as a collapsed, attached type converter, based on its connections to other nodes.
