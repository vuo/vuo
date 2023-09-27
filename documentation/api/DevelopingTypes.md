@addtogroup DevelopingTypes

In a node, each data-and-event port has a [data type](https://doc.vuo.org/@vuoVersion/manual/how-compositions-process-data.xhtml). Vuo comes with many built-in data types, such as @ref VuoInteger, @ref VuoReal, @ref VuoText, and other @ref VuoTypes.

Occasionally when developing a node class you may find that the built-in types are not enough, and you need to create your own custom type. This type would allow you to pass structured data from an output port on one node to an input port on another. The data structure would likely be specific to your node set. For example, a node set that interfaced with an internet-enabled coffee maker might need a data type for the coffee maker device.

(If you don't need to pass structured data between nodes, but just need an input port to show a menu of options, then you don't have to define a custom type. You can just specify `menuItems` in the port's @ref VuoInputData.)

This documentation assumes that you're already familiar with @ref DevelopingNodeClasses.

## Getting started

The easiest way to start developing a data type is with the example project provided in the [Vuo SDK](https://doc.vuo.org/@vuoVersion/manual/the-command-line-tools.xhtml#installing-the-vuo-sdk).

   1. Copy `/Library/Developer/Vuo/example/node/customType` to your Desktop.
   2. Rename `ExampleLanguage.h` and `ExampleLanguage.c` to `MyLanguage.h` and `MyLanguage.c`.
   3. In `CMakeLists.txt`, change `ExampleLanguage.h` and `ExampleLanguage.c` to `MyLanguage.h` and `MyLanguage.c`.
   4. In `example.customType.greet.c`, replace all occurrences of `ExampleLanguage` with `MyLanguage`.
   5. In `ExampleLanguage.c`, in the @ref VuoModuleMetadata call, change "Language" to "My Language". Save the file.
   6. Build the project using Qt Creator or the command line.

The `example.customType.greet` node class should now be listed in Vuo's Node Library. When you drag the node onto the canvas and click on its input port, the port popover should show the port's data type as "My Language".

## Writing a data type

To implement a data type, you need to:

   - Create the type's header file and implementation file.
   - Call @ref VuoModuleMetadata to define the type's metadata.
   - Declare the type's underlying data structure.
   - Declare and implement Vuo API functions to define the type's behavior.

### The file name

Every data type has two names: a human-readable title and a machine name. The title appears in the port popover, for example `Integer` or `My Language`. The machine name is how the type is referred to in the source code, for example @ref VuoInteger or `MyLanguage`. When you create a data type, the file names consist of the machine name plus a file extension.

Like node classes, types can be implemented in C/C++/Objective-C, and the file extension for the implementation file should reflect the language. The file extension for the header file should always be `.h`.

### The metadata

Here's an example of metadata for a data type:

@code{.c}
VuoModuleMetadata({
    "title" : "Audio Samples",
    "version" : "1.0.0",
    "dependencies" : [
        "VuoInteger",
        "VuoReal",
        "VuoText"
    ]
});
@endcode

Like node classes, types must pass the @ref VuoModuleMetadata macro a JSON-formatted argument with certain keys. For more information, see the documentation for @ref VuoModuleMetadata.

### The underlying data structure

In the simplest case, your data type may just store data of an existing C/C++/Objective-C type or Vuo type. For example:

@code{.c}
// VuoInteger.h
typedef int64_t VuoInteger;

// VuoUrl.h
typedef VuoText VuoUrl;
@endcode

Or your data type may consist of a data structure composed from multiple types. For example:

@code{.c}
// VuoColor.h
typedef struct
{
    float r,g,b,a;
} VuoColor;

// VuoAudioSamples.h
typedef struct
{
    VuoInteger sampleCount;
    VuoReal *samples;
    VuoReal samplesPerSecond;
} VuoAudioSamples;
@endcode

If your type refers to other Vuo types, then you need to list those as dependencies in the @ref VuoModuleMetadata. For more information, see @ref ManagingDependencies.

### The functions

To implement a data type, there are certain Vuo API functions that you need to declare and implement.

Unlike in a node class, in a type you need to prefix Vuo API function names with your type name. In the following documentation, you would substitute your type's machine name (e.g. `MyLanguage`) in place of `MyType`.

There are 3 functions that you must implement:

   - @ref MyType_makeFromJson — Vuo calls this function when converting JSON-formatted text to a `MyType` value, such as when the user edits the value of a `MyType` port in an input editor. A serialized JSON-formatted value of type @ref VuoPoint2d, for example, might look like `{"x":0.5,"y":1.2}`. The argument to this function is a [JSON-C](https://github.com/json-c/json-c) object containing a serialized representation of a `MyType` value. Your implementation of this function should convert the serialized representation to an instance of `MyType`.
   - @ref MyType_getJson — This function is the inverse of @ref MyType_makeFromJson. Your implementation of this function should convert a `MyType` value to a JSON-C object.
   - @ref MyType_getSummary — Vuo calls this function when the user opens a port popover to see the current value of a `MyType` port. Your implementation of this function should return a human-readable description of the given `MyType` value.

In addition to these required functions, there are several that you can optionally implement. There are also some functions that you can declare but don't have to implement because Vuo generates them automatically. For more information, see @ref VuoTypeMethods.

If you're writing the data type in C++, you should wrap your Vuo API function declarations and @ref VuoModuleMetadata call in `extern "C"`.

See the source code for Vuo's built-in data types — in the [core types directory](https://github.com/vuo/vuo/tree/main/type) and in [node set directories](https://github.com/vuo/vuo/tree/main/node) — which can serve as examples to help you write your own.

## Compiling and installing a data type

To be able to use your data type within Vuo, you'll need to:

   - Use the `vuo-compile` command to compile the data type to a `.bc` file.
   - Package the data type into a node set that contains the type's compiled (`.bc`) file and header (`.h`) file.
   - Place the node set (`.vuonode`) file in one of Vuo's [Modules folders](https://doc.vuo.org/@vuoVersion/manual/installing-a-node.xhtml).

The example project in `/Library/Developer/Vuo/example/node/customType` performs all of these steps. You can use that example's `CMakeLists.txt` as a starting point when creating your own type.

When you build the `customType` example project, it places the node set in the User Modules folder. For more information about node sets, see @ref PackagingNodeSets.

## Naming data types

**Please do not begin your type's name with "Vuo".** This is reserved for types distributed by Team Vuo / Kosada. Please use your own company or personal name for your types so that Vuo users can appreciate your work (and not be confused).

Built-in Vuo types follow a set of naming conventions. If you develop types to share with other Vuo developers, we encourage you to follow these conventions, too, to make your types easier to use.

   - A type's name should:
      - Use upper camel case (`VuoVideoFrame`).
      - Capitalize only the first letter of abbreviations (`VuoMidiNote`, not `VuoMIDINote`).
   - A type function's name should:
      - Be prefixed with the type name and an underscore (`VuoPoint2d_dotProduct()`).
      - For the constructor function, be called "make" (`VuoPoint2d_make()`).
